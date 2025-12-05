//
// Code generation context utilities
//
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "IRbuilder.h"
#include "ast/ast.h"

namespace front::ir {
    struct Binding {
        Value *address{nullptr};
        ast::BasicType type{ast::BasicType::Int};
        bool is_const{false};
        bool is_global{false};
    };

    struct FunctionInfo {
        Function *function{nullptr};
        ast::BasicType return_type{ast::BasicType::Void};
        std::vector<ast::BasicType> param_types;
    };

    class CodegenContext {
    public:
        explicit CodegenContext(std::unique_ptr<Module> module);

        Module &module();

        const Module &module() const;

        IRBuilder &builder() const;

        [[nodiscard]] bool has_builder() const noexcept;

        void set_insert_point(BasicBlock *block);

        void push_scope();

        void pop_scope();

        void bind(const std::string &name, Binding binding);

        Binding *lookup(const std::string &name);

        const Binding *lookup(const std::string &name) const;

        Type *to_ir_type(ast::BasicType type) const;

        FunctionInfo &declare_function(const ast::FuncDef &def);

        FunctionInfo *find_function(const std::string &name);

        const FunctionInfo *find_function(const std::string &name) const;

        Value *make_int(int value);

        Value *make_float(float value);

        Value *make_bool(bool value) const;

        Value *as_bool(Value *value);

        Value *as_int(Value *value);

        Value *as_float(Value *value);

        Value *cast(Value *value, ast::BasicType target);

        BasicBlock *create_block(std::string_view base_name);

        std::unique_ptr<Module> module_ptr;
        std::unique_ptr<IRBuilder> builder_ptr;
        Function *current_function{nullptr};
        std::optional<ast::BasicType> current_return_type{};

    private:
        std::vector<std::unordered_map<std::string, Binding> > scopes_;
        std::unordered_map<std::string, FunctionInfo> functions_;
        int block_seq_{0};
    };
}
