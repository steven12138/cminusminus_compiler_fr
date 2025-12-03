//
// Created by steven on 12/3/25.
//

#include "ast/ast_builder.h"
#include <stdexcept>

namespace front::ast {

    // Utils
    void add_to_program(Program &prog, SemVal &item) {
        if (std::holds_alternative<DeclPtr>(item)) {
            prog.globals.push_back(std::move(std::get<DeclPtr>(item)));
        } else if (std::holds_alternative<FuncPtr>(item)) {
            prog.functions.push_back(std::move(std::get<FuncPtr>(item)));
        }
    }

    SemVal build_single_forward(std::vector<SemVal> &rhs) {
        return std::move(rhs[0]);
    }

    // Types 
    SemVal build_type_int(std::vector<SemVal> &) { return BasicType::Int; }
    SemVal build_type_float(std::vector<SemVal> &) { return BasicType::Float; }
    SemVal build_type_void(std::vector<SemVal> &) { return BasicType::Void; }

    // Program 
    SemVal build_comp_unit_list_item(std::vector<SemVal> &rhs) {
        auto prog = std::make_unique<Program>();
        add_to_program(*prog, rhs[0]);
        return ProgramPtr(std::move(prog));
    }

    SemVal build_comp_unit_list_append(std::vector<SemVal> &rhs) {
        auto prog = std::move(std::get<ProgramPtr>(rhs[0]));
        add_to_program(*prog, rhs[1]);
        return ProgramPtr(std::move(prog));
    }

    // Declarations 
    SemVal build_const_decl(std::vector<SemVal> &rhs) {
        auto decl = std::make_unique<VarDecl>();
        decl->is_const = true;
        decl->type = std::get<BasicType>(rhs[1]);
        decl->items = std::move(std::get<std::vector<VarInit> >(rhs[2]));

        DeclPtr ptr = std::move(decl);
        return ptr;
    }

    SemVal build_var_decl(std::vector<SemVal> &rhs) {
        auto decl = std::make_unique<VarDecl>();
        decl->is_const = false;
        decl->type = std::get<BasicType>(rhs[0]);
        decl->items = std::move(std::get<std::vector<VarInit> >(rhs[1]));

        DeclPtr ptr = std::move(decl);
        return ptr;
    }

    SemVal build_def_list_item(std::vector<SemVal> &rhs) {
        return std::move(rhs[0]);
    }

    SemVal build_def_list_append(std::vector<SemVal> &rhs) {
        auto list = std::move(std::get<std::vector<VarInit> >(rhs[0]));
        auto item_vec = std::move(std::get<std::vector<VarInit> >(rhs[2]));
        list.insert(list.end(), std::make_move_iterator(item_vec.begin()), std::make_move_iterator(item_vec.end()));
        return list;
    }

    SemVal build_const_def(std::vector<SemVal> &rhs) {
        VarInit init;
        init.name = std::get<std::string>(rhs[0]);
        init.value = std::move(std::get<ExprPtr>(rhs[2]));

        std::vector<VarInit> vec;
        vec.push_back(std::move(init));
        return vec;
    }

    SemVal build_var_def_uninit(std::vector<SemVal> &rhs) {
        VarInit init;
        init.name = std::get<std::string>(rhs[0]);
        init.value = nullptr;

        std::vector<VarInit> vec;
        vec.push_back(std::move(init));
        return vec;
    }

    SemVal build_var_def_init(std::vector<SemVal> &rhs) {
        VarInit init;
        init.name = std::get<std::string>(rhs[0]);
        init.value = std::move(std::get<ExprPtr>(rhs[2]));

        std::vector<VarInit> vec;
        vec.push_back(std::move(init));
        return vec;
    }

    // Functions 
    SemVal build_func_def(std::vector<SemVal> &rhs) {
        auto func = std::make_unique<FuncDef>();
        func->type = std::get<BasicType>(rhs[0]);
        func->name = std::get<std::string>(rhs[1]);
        func->params = std::move(std::get<std::vector<Param> >(rhs[3]));
        func->body = std::move(std::get<BlockPtr>(rhs[5]));

        FuncPtr ptr = std::move(func);
        return ptr;
    }

    SemVal build_func_def_no_params(std::vector<SemVal> &rhs) {
        auto func = std::make_unique<FuncDef>();
        func->type = std::get<BasicType>(rhs[0]);
        func->name = std::get<std::string>(rhs[1]);
        func->body = std::move(std::get<BlockPtr>(rhs[4]));

        FuncPtr ptr = std::move(func);
        return ptr;
    }

    SemVal build_func_fparams_item(std::vector<SemVal> &rhs) {
        return std::move(rhs[0]);
    }

    SemVal build_func_fparams_append(std::vector<SemVal> &rhs) {
        auto list = std::move(std::get<std::vector<Param> >(rhs[0]));
        auto item_vec = std::move(std::get<std::vector<Param> >(rhs[2]));
        list.insert(list.end(), std::make_move_iterator(item_vec.begin()), std::make_move_iterator(item_vec.end()));
        return list;
    }

    SemVal build_func_fparam(std::vector<SemVal> &rhs) {
        Param p;
        p.type = std::get<BasicType>(rhs[0]);
        p.name = std::get<std::string>(rhs[1]);
        std::vector<Param> vec;
        vec.push_back(std::move(p));
        return vec;
    }

    // Blocks 
    // Strategy: Reuse BlockPtr (BlockStmt*) to accumulate items in BlockItemList rules
    // since vector<BlockItem> is not in SemVal.

    SemVal build_block(std::vector<SemVal> &rhs) {
        // Block -> { BlockItemList }
        // rhs[1] is already a fully formed BlockPtr from the list rules
        return std::move(rhs[1]);
    }

    SemVal build_block_empty(std::vector<SemVal> &) {
        return BlockPtr(std::make_unique<BlockStmt>());
    }

    SemVal build_block_item_list_item(std::vector<SemVal> &rhs) {
        // BlockItemList -> BlockItem
        // Create a new BlockStmt and add the single item
        auto block = std::make_unique<BlockStmt>();
        block->items.push_back(std::move(std::get<BlockItem>(rhs[0])));
        return BlockPtr(std::move(block));
    }

    SemVal build_block_item_list_append(std::vector<SemVal> &rhs) {
        // BlockItemList -> BlockItemList BlockItem
        // Take existing BlockPtr and append item
        auto block = std::move(std::get<BlockPtr>(rhs[0]));
        block->items.push_back(std::move(std::get<BlockItem>(rhs[1])));
        return BlockPtr(std::move(block));
    }

    SemVal build_block_item_decl(std::vector<SemVal> &rhs) {
        return BlockItem::make_decl(std::move(std::get<DeclPtr>(rhs[0])));
    }

    SemVal build_block_item_stmt(std::vector<SemVal> &rhs) {
        return BlockItem::make_stmt(std::move(std::get<StmtPtr>(rhs[0])));
    }

    // Statements 
    SemVal build_stmt_assign(std::vector<SemVal> &rhs) {
        auto stmt = std::make_unique<AssignStmt>();
        stmt->target = std::get<std::string>(rhs[0]);
        stmt->expr = std::move(std::get<ExprPtr>(rhs[2]));

        StmtPtr ptr = std::move(stmt);
        return ptr;
    }

    SemVal build_stmt_exp(std::vector<SemVal> &rhs) {
        auto stmt = std::make_unique<ExprStmt>();
        stmt->expr = std::move(std::get<ExprPtr>(rhs[0]));

        StmtPtr ptr = std::move(stmt);
        return ptr;
    }

    SemVal build_stmt_empty(std::vector<SemVal> &) {
        StmtPtr ptr = std::make_unique<EmptyStmt>();
        return ptr;
    }

    SemVal build_stmt_if(std::vector<SemVal> &rhs) {
        auto stmt = std::make_unique<IfStmt>();
        stmt->condition = std::move(std::get<ExprPtr>(rhs[2]));
        stmt->then_branch = std::move(std::get<StmtPtr>(rhs[4]));

        StmtPtr ptr = std::move(stmt);
        return ptr;
    }

    SemVal build_stmt_if_else(std::vector<SemVal> &rhs) {
        auto stmt = std::make_unique<IfStmt>();
        stmt->condition = std::move(std::get<ExprPtr>(rhs[2]));
        stmt->then_branch = std::move(std::get<StmtPtr>(rhs[4]));
        stmt->else_branch = std::move(std::get<StmtPtr>(rhs[6]));

        StmtPtr ptr = std::move(stmt);
        return ptr;
    }

    SemVal build_stmt_return(std::vector<SemVal> &rhs) {
        auto stmt = std::make_unique<ReturnStmt>();
        stmt->value = std::move(std::get<ExprPtr>(rhs[1]));

        StmtPtr ptr = std::move(stmt);
        return ptr;
    }

    SemVal build_stmt_return_void(std::vector<SemVal> &) {
        auto stmt = std::make_unique<ReturnStmt>();
        stmt->value = nullptr;

        StmtPtr ptr = std::move(stmt);
        return ptr;
    }

    // Expressions 
    SemVal build_exp_int(std::vector<SemVal> &rhs) {
        int v = std::get<int>(rhs[0]);
        auto node = std::make_unique<LiteralInt>();
        node->value = v;

        ExprPtr ptr = std::move(node);
        return ptr;
    }

    SemVal build_exp_float(std::vector<SemVal> &rhs) {
        float v = std::get<float>(rhs[0]);
        auto node = std::make_unique<LiteralFloat>();
        node->value = v;

        return ExprPtr{std::move(node)};
    }

    SemVal build_lval_ident(std::vector<SemVal> &rhs) {
        return std::move(std::get<std::string>(rhs[0]));
    }

    SemVal build_exp_lval(std::vector<SemVal> &rhs) {
        auto node = std::make_unique<IdentifierExpr>();
        node->name = std::get<std::string>(rhs[0]);

        return ExprPtr{std::move(node)};
    }

    SemVal build_func_rparams_item(std::vector<SemVal> &rhs) {
        VarInit wrapper;
        wrapper.value = std::move(std::get<ExprPtr>(rhs[0]));
        std::vector<VarInit> vec;
        vec.push_back(std::move(wrapper));
        return vec;
    }

    SemVal build_func_rparams_append(std::vector<SemVal> &rhs) {
        auto list = std::move(std::get<std::vector<VarInit> >(rhs[0]));
        auto item_vec = std::move(std::get<std::vector<VarInit> >(rhs[2]));
        list.insert(list.end(), std::make_move_iterator(item_vec.begin()), std::make_move_iterator(item_vec.end()));
        return list;
    }

    SemVal build_exp_call(std::vector<SemVal> &rhs) {
        auto node = std::make_unique<CallExpr>();
        node->callee = std::get<std::string>(rhs[0]);

        if (std::holds_alternative<std::vector<VarInit> >(rhs[2])) {
            auto wrappers = std::move(std::get<std::vector<VarInit> >(rhs[2]));
            for (auto &w: wrappers) {
                node->args.push_back(std::move(w.value));
            }
        }

        ExprPtr ptr = std::move(node);
        return ptr;
    }

    SemVal build_exp_call_void(std::vector<SemVal> &rhs) {
        auto node = std::make_unique<CallExpr>();
        node->callee = std::get<std::string>(rhs[0]);

        ExprPtr ptr = std::move(node);
        return ptr;
    }

    SemVal build_unary_op_positive(std::vector<SemVal> &) { return UnaryOp::Positive; }
    SemVal build_unary_op_negative(std::vector<SemVal> &) { return UnaryOp::Negative; }
    SemVal build_unary_op_not(std::vector<SemVal> &) { return UnaryOp::LogicalNot; }

    SemVal build_unary_exp(std::vector<SemVal> &rhs) {
        auto node = std::make_unique<UnaryExpr>();
        node->op = std::get<UnaryOp>(rhs[0]);
        node->operand = std::move(std::get<ExprPtr>(rhs[1]));

        ExprPtr ptr = std::move(node);
        return ptr;
    }

    SemVal make_binary(BasicOp op, std::vector<SemVal> &rhs) {
        auto node = std::make_unique<BinaryExpr>();
        node->op = op;
        node->lhs = std::move(std::get<ExprPtr>(rhs[0]));
        node->rhs = std::move(std::get<ExprPtr>(rhs[2]));

        ExprPtr ptr = std::move(node);
        return ptr;
    }

    SemVal build_binary_add(std::vector<SemVal> &rhs) { return make_binary(BasicOp::Add, rhs); }
    SemVal build_binary_sub(std::vector<SemVal> &rhs) { return make_binary(BasicOp::Sub, rhs); }
    SemVal build_binary_mul(std::vector<SemVal> &rhs) { return make_binary(BasicOp::Mul, rhs); }
    SemVal build_binary_div(std::vector<SemVal> &rhs) { return make_binary(BasicOp::Div, rhs); }
    SemVal build_binary_mod(std::vector<SemVal> &rhs) { return make_binary(BasicOp::Mod, rhs); }
    SemVal build_binary_lt(std::vector<SemVal> &rhs) { return make_binary(BasicOp::Lt, rhs); }
    SemVal build_binary_gt(std::vector<SemVal> &rhs) { return make_binary(BasicOp::Gt, rhs); }
    SemVal build_binary_le(std::vector<SemVal> &rhs) { return make_binary(BasicOp::Le, rhs); }
    SemVal build_binary_ge(std::vector<SemVal> &rhs) { return make_binary(BasicOp::Ge, rhs); }
    SemVal build_binary_eq(std::vector<SemVal> &rhs) { return make_binary(BasicOp::Eq, rhs); }
    SemVal build_binary_neq(std::vector<SemVal> &rhs) { return make_binary(BasicOp::Neq, rhs); }
    SemVal build_binary_and(std::vector<SemVal> &rhs) { return make_binary(BasicOp::And, rhs); }
    SemVal build_binary_or(std::vector<SemVal> &rhs) { return make_binary(BasicOp::Or, rhs); }
}
