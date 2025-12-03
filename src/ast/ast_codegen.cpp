//
// AST-driven LLVM-like IR generation.
//
#include "ast/ast.h"
#include "ir/codegen_context.h"

#include <optional>
#include <stdexcept>
#include <utility>

#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "GlobalVariable.h"
#include "Instruction.h"
#include "Module.h"
#include "Type.h"

namespace front::ir {
    using namespace ast;

    struct ScopeGuard {
        CodegenContext &ctx;
        bool active{true};

        explicit ScopeGuard(CodegenContext &ctx) : ctx(ctx) {
            ctx.push_scope();
        }

        ~ScopeGuard() {
            if (active) {
                ctx.pop_scope();
            }
        }

        void release() { active = false; }
    };

    bool has_terminator(BasicBlock *bb) {
        return bb && bb->get_terminator() != nullptr;
    }

    std::optional<int> eval_int_constant(const Expr *expr) {
        if (const auto *lit = dynamic_cast<const LiteralInt *>(expr)) {
            return lit->value;
        }
        if (const auto *unary = dynamic_cast<const UnaryExpr *>(expr)) {
            auto inner = eval_int_constant(unary->operand.get());
            if (!inner) {
                return std::nullopt;
            }
            switch (unary->op) {
                case UnaryOp::Positive:
                    return *inner;
                case UnaryOp::Negative:
                    return -*inner;
                case UnaryOp::LogicalNot:
                    return *inner == 0 ? 1 : 0;
            }
        }
        if (const auto *binary = dynamic_cast<const BinaryExpr *>(expr)) {
            auto lhs = eval_int_constant(binary->lhs.get());
            auto rhs = eval_int_constant(binary->rhs.get());
            if (!lhs || !rhs) {
                return std::nullopt;
            }
            switch (binary->op) {
                case BasicOp::Add: return *lhs + *rhs;
                case BasicOp::Sub: return *lhs - *rhs;
                case BasicOp::Mul: return *lhs * *rhs;
                case BasicOp::Div: return *rhs == 0 ? 0 : *lhs / *rhs;
                case BasicOp::Mod: return *rhs == 0 ? 0 : *lhs % *rhs;
                case BasicOp::Lt: return *lhs < *rhs;
                case BasicOp::Gt: return *lhs > *rhs;
                case BasicOp::Le: return *lhs <= *rhs;
                case BasicOp::Ge: return *lhs >= *rhs;
                case BasicOp::Eq: return *lhs == *rhs;
                case BasicOp::Neq: return *lhs != *rhs;
                case BasicOp::And: return (*lhs != 0) && (*rhs != 0);
                case BasicOp::Or: return (*lhs != 0) || (*rhs != 0);
            }
        }
        return std::nullopt;
    }


    CodegenContext::CodegenContext(std::unique_ptr<Module> module)
        : module_ptr(std::move(module)) {
        push_scope();
    }

    Module &CodegenContext::module() {
        return *module_ptr;
    }

    const Module &CodegenContext::module() const {
        return *module_ptr;
    }

    IRBuilder &CodegenContext::builder() const {
        if (!builder_ptr) {
            throw std::runtime_error("IRBuilder has no insert point yet");
        }
        return *builder_ptr;
    }

    bool CodegenContext::has_builder() const noexcept {
        return static_cast<bool>(builder_ptr);
    }

    void CodegenContext::set_insert_point(BasicBlock *block) {
        if (builder_ptr) {
            builder_ptr->set_insert_point(block);
        } else {
            builder_ptr = std::make_unique<IRBuilder>(block, module_ptr.get());
        }
    }

    void CodegenContext::push_scope() {
        scopes_.emplace_back();
    }

    void CodegenContext::pop_scope() {
        if (scopes_.empty()) {
            throw std::runtime_error("Attempted to pop an empty scope stack");
        }
        scopes_.pop_back();
    }

    void CodegenContext::bind(const std::string &name, Binding binding) {
        if (scopes_.empty()) {
            throw std::runtime_error("No active scope to bind variable " + name);
        }
        scopes_.back()[name] = binding;
    }

    Binding *CodegenContext::lookup(const std::string &name) {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return &found->second;
            }
        }
        return nullptr;
    }

    const Binding *CodegenContext::lookup(const std::string &name) const {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return &found->second;
            }
        }
        return nullptr;
    }

    Type *CodegenContext::to_ir_type(BasicType type) const {
        switch (type) {
            case BasicType::Int:
                return module_ptr->get_int32_type();
            case BasicType::Void:
                return module_ptr->get_void_type();
            case BasicType::Float:
                return module_ptr->get_float_type();
        }
        throw std::runtime_error("Unsupported basic type");
    }

    FunctionInfo &CodegenContext::declare_function(const FuncDef &def) {
        if (const auto existing = functions_.find(def.name); existing != functions_.end()) {
            return existing->second;
        }

        FunctionInfo info{};
        info.return_type = def.type;

        std::vector<Type *> param_types;
        param_types.reserve(def.params.size());
        info.param_types.reserve(def.params.size());
        for (const auto &param: def.params) {
            param_types.push_back(to_ir_type(param.type));
            info.param_types.push_back(param.type);
        }

        const auto func_type = FunctionType::get(to_ir_type(def.type), param_types);
        info.function = Function::create(func_type, def.name, module_ptr.get());
        const auto [it, inserted] = functions_.emplace(def.name, std::move(info));
        if (!inserted) {
            throw std::runtime_error("Failed to insert function: " + def.name);
        }
        return it->second;
    }

    FunctionInfo *CodegenContext::find_function(const std::string &name) {
        if (const auto it = functions_.find(name); it != functions_.end()) {
            return &it->second;
        }
        return nullptr;
    }

    const FunctionInfo *CodegenContext::find_function(const std::string &name) const {
        if (auto it = functions_.find(name); it != functions_.end()) {
            return &it->second;
        }
        return nullptr;
    }

    Value *CodegenContext::make_int(int value) {
        return ConstantInt::get(value, module_ptr.get());
    }

    Value *CodegenContext::make_bool(bool value) const {
        return ConstantInt::get(value, module_ptr.get());
    }

    Value *CodegenContext::as_bool(Value *value) {
        if (value->get_type()->is_int1_type()) {
            return value;
        }
        if (value->get_type()->is_int32_type()) {
            return builder().create_icmp_ne(value, make_int(0));
        }
        throw std::runtime_error("Cannot convert value to bool");
    }

    Value *CodegenContext::as_int(Value *value) {
        if (value->get_type()->is_int32_type()) {
            return value;
        }
        if (value->get_type()->is_int1_type()) {
            return builder().create_zext(value, module_ptr->get_int32_type());
        }
        throw std::runtime_error("Cannot convert value to int32");
    }

    Value *CodegenContext::cast(Value *value, BasicType target) {
        switch (target) {
            case BasicType::Int:
                return as_int(value);
            case BasicType::Void:
                if (value != nullptr) {
                    throw std::runtime_error("Cannot cast non-void value to void");
                }
                return value;
            case BasicType::Float:
                throw std::runtime_error("Float lowering is not implemented");
        }
        throw std::runtime_error("Unknown cast target");
    }

    BasicBlock *CodegenContext::create_block(std::string_view base_name) {
        if (current_function == nullptr) {
            throw std::runtime_error("Cannot create block without active function");
        }
        std::string name(base_name);
        name += ".";
        name += std::to_string(block_seq_++);
        return BasicBlock::create(module_ptr.get(), name, current_function);
    }
}

namespace front::ast {
    using namespace ir;

    Value *LiteralInt::codegen(CodegenContext &ctx) {
        return ctx.make_int(value);
    }

    Value *LiteralFloat::codegen(CodegenContext &) {
        throw std::runtime_error("Float literal codegen is not implemented");
    }

    Value *IdentifierExpr::codegen(CodegenContext &ctx) {
        auto *binding = ctx.lookup(name);
        if (!binding) {
            throw std::runtime_error("Undefined identifier: " + name);
        }
        return ctx.builder().create_load(binding->address);
    }

    Value *UnaryExpr::codegen(CodegenContext &ctx) {
        auto *operand_val = operand->codegen(ctx);
        switch (op) {
            case UnaryOp::Positive:
                return ctx.as_int(operand_val);
            case UnaryOp::Negative: {
                auto *zero = ctx.make_int(0);
                return ctx.builder().create_isub(zero, ctx.as_int(operand_val));
            }
            case UnaryOp::LogicalNot: {
                auto *cond = ctx.as_bool(operand_val);
                return ctx.builder().create_icmp_eq(cond, ctx.make_bool(false));
            }
        }
        throw std::runtime_error("Unhandled unary op");
    }

    Value *BinaryExpr::codegen(CodegenContext &ctx) {
        if (op == BasicOp::And || op == BasicOp::Or) {
            auto *lhs_cond = ctx.as_bool(lhs->codegen(ctx));
            auto *origin_block = ctx.builder().get_insert_block();
            auto *rhs_block = ctx.create_block(op == BasicOp::And ? "and.rhs" : "or.rhs");
            auto *merge_block = ctx.create_block(op == BasicOp::And ? "and.merge" : "or.merge");

            if (op == BasicOp::And) {
                ctx.builder().create_cond_br(lhs_cond, rhs_block, merge_block);
            } else {
                ctx.builder().create_cond_br(lhs_cond, merge_block, rhs_block);
            }

            ctx.set_insert_point(rhs_block);
            auto *rhs_cond = ctx.as_bool(rhs->codegen(ctx));
            ctx.builder().create_br(merge_block);
            auto *rhs_end = ctx.builder().get_insert_block();

            ctx.set_insert_point(merge_block);
            auto *phi = PhiInst::create_phi(ctx.module().get_int1_type(), merge_block);
            merge_block->add_instr_begin(phi);
            if (op == BasicOp::And) {
                phi->add_phi_pair_operand(rhs_cond, rhs_end);
                phi->add_phi_pair_operand(ctx.make_bool(false), origin_block);
            } else {
                phi->add_phi_pair_operand(ctx.make_bool(true), origin_block);
                phi->add_phi_pair_operand(rhs_cond, rhs_end);
            }
            return phi;
        }

        auto *lhs_val = lhs->codegen(ctx);
        auto *rhs_val = rhs->codegen(ctx);
        switch (op) {
            case BasicOp::Add:
                return ctx.builder().create_iadd(ctx.as_int(lhs_val), ctx.as_int(rhs_val));
            case BasicOp::Sub:
                return ctx.builder().create_isub(ctx.as_int(lhs_val), ctx.as_int(rhs_val));
            case BasicOp::Mul:
                return ctx.builder().create_imul(ctx.as_int(lhs_val), ctx.as_int(rhs_val));
            case BasicOp::Div:
                return ctx.builder().create_isdiv(ctx.as_int(lhs_val), ctx.as_int(rhs_val));
            case BasicOp::Mod:
                return ctx.builder().create_irem(ctx.as_int(lhs_val), ctx.as_int(rhs_val));
            case BasicOp::Lt:
                return ctx.builder().create_icmp_lt(ctx.as_int(lhs_val), ctx.as_int(rhs_val));
            case BasicOp::Gt:
                return ctx.builder().create_icmp_gt(ctx.as_int(lhs_val), ctx.as_int(rhs_val));
            case BasicOp::Le:
                return ctx.builder().create_icmp_le(ctx.as_int(lhs_val), ctx.as_int(rhs_val));
            case BasicOp::Ge:
                return ctx.builder().create_icmp_ge(ctx.as_int(lhs_val), ctx.as_int(rhs_val));
            case BasicOp::Eq:
                return ctx.builder().create_icmp_eq(ctx.as_int(lhs_val), ctx.as_int(rhs_val));
            case BasicOp::Neq:
                return ctx.builder().create_icmp_ne(ctx.as_int(lhs_val), ctx.as_int(rhs_val));
            case BasicOp::And:
            case BasicOp::Or:
                break;
        }
        throw std::runtime_error("Unhandled binary operation");
    }

    Value *CallExpr::codegen(CodegenContext &ctx) {
        auto *info = ctx.find_function(callee);
        if (!info) {
            throw std::runtime_error("Unknown function: " + callee);
        }
        if (info->param_types.size() != args.size()) {
            throw std::runtime_error("Argument count mismatch for " + callee);
        }

        std::vector<Value *> arg_values;
        arg_values.reserve(args.size());
        for (std::size_t i = 0; i < args.size(); ++i) {
            auto *val = args[i]->codegen(ctx);
            arg_values.push_back(ctx.cast(val, info->param_types[i]));
        }
        return ctx.builder().create_call(info->function, arg_values);
    }

    void EmptyStmt::codegen(CodegenContext &) {
    }

    void ExprStmt::codegen(CodegenContext &ctx) {
        if (expr) {
            expr->codegen(ctx);
        }
    }

    void AssignStmt::codegen(CodegenContext &ctx) {
        auto *binding = ctx.lookup(target);
        if (!binding) {
            throw std::runtime_error("Assignment to undefined variable: " + target);
        }
        if (binding->is_const) {
            throw std::runtime_error("Assignment to const variable: " + target);
        }
        auto *val = expr->codegen(ctx);
        ctx.builder().create_store(ctx.cast(val, binding->type), binding->address);
    }

    void ReturnStmt::codegen(CodegenContext &ctx) {
        if (!ctx.current_return_type) {
            throw std::runtime_error("Return used outside of a function");
        }
        if (*ctx.current_return_type == BasicType::Void) {
            ctx.builder().create_void_ret();
            return;
        }
        if (!value) {
            ctx.builder().create_ret(ctx.make_int(0));
            return;
        }
        auto *val = ctx.cast(value->codegen(ctx), *ctx.current_return_type);
        ctx.builder().create_ret(val);
    }

    void IfStmt::codegen(CodegenContext &ctx) {
        auto *cond_val = ctx.as_bool(condition->codegen(ctx));
        auto *then_bb = ctx.create_block("if.then");
        auto *merge_bb = ctx.create_block("if.end");
        BasicBlock *else_bb = else_branch ? ctx.create_block("if.else") : merge_bb;

        ctx.builder().create_cond_br(cond_val, then_bb, else_bb);

        ctx.set_insert_point(then_bb);
        then_branch->codegen(ctx);
        if (!has_terminator(then_bb)) {
            ctx.builder().create_br(merge_bb);
        }

        if (else_branch) {
            ctx.set_insert_point(else_bb);
            else_branch->codegen(ctx);
            if (!has_terminator(else_bb)) {
                ctx.builder().create_br(merge_bb);
            }
        }

        ctx.set_insert_point(merge_bb);
    }

    void BlockStmt::codegen(CodegenContext &ctx) {
        ScopeGuard scope(ctx);
        for (auto &item: items) {
            if (item.type == BlockItem::Type::Decl && item.decl) {
                item.decl->codegen(ctx);
            } else if (item.type == BlockItem::Type::Stmt && item.stmt) {
                item.stmt->codegen(ctx);
            }
        }
    }

    void VarDecl::codegen(CodegenContext &ctx) {
        const auto ir_type = ctx.to_ir_type(type);
        if (ctx.current_function == nullptr) {
            for (auto &init: items) {
                Constant *initializer = nullptr;
                if (init.value) {
                    const auto folded = eval_int_constant(init.value.get());
                    if (!folded) {
                        throw std::runtime_error("Global initializers must be constant: " + init.name);
                    }
                    initializer = ConstantInt::get(*folded, &ctx.module());
                } else {
                    initializer = ConstantZero::get(ir_type, &ctx.module());
                }
                auto *global = GlobalVariable::create(init.name, &ctx.module(), ir_type, is_const, initializer);
                ctx.bind(init.name, Binding{global, type, is_const, true});
            }
            return;
        }

        for (auto &init: items) {
            auto *alloca = ctx.builder().create_alloca(ir_type);
            ctx.bind(init.name, Binding{alloca, type, is_const, false});
            if (init.value) {
                auto *val = ctx.cast(init.value->codegen(ctx), type);
                ctx.builder().create_store(val, alloca);
            }
        }
    }

    void FuncDef::codegen(CodegenContext &ctx) {
        const auto &info = ctx.declare_function(*this);
        auto *func = info.function;

        const auto previous_function = ctx.current_function;
        const auto previous_return = ctx.current_return_type;
        ctx.current_function = func;
        ctx.current_return_type = type;

        ScopeGuard scope(ctx);

        auto *entry = BasicBlock::create(&ctx.module(), "entry", func);
        ctx.set_insert_point(entry);

        auto arg_it = func->arg_begin();
        for (const auto &param: params) {
            if (arg_it == func->arg_end()) {
                throw std::runtime_error("Parameter count mismatch for " + name);
            }
            auto *alloca = ctx.builder().create_alloca(ctx.to_ir_type(param.type));
            ctx.bind(param.name, Binding{alloca, param.type, false, false});
            ctx.builder().create_store(*arg_it, alloca);
            ++arg_it;
        }

        if (body) {
            body->codegen(ctx);
        }

        auto *tail_block = ctx.builder().get_insert_block();
        if (tail_block && !has_terminator(tail_block)) {
            if (type == BasicType::Void) {
                ctx.builder().create_void_ret();
            } else {
                ctx.builder().create_ret(ctx.make_int(0));
            }
        }

        ctx.current_function = previous_function;
        ctx.current_return_type = previous_return;
    }

    void Program::codegen(CodegenContext &ctx) const {
        for (auto &decl: globals) {
            decl->codegen(ctx);
        }
        for (const auto &fn: functions) {
            ctx.declare_function(*fn);
        }
        for (auto &fn: functions) {
            fn->codegen(ctx);
        }
    }
}
