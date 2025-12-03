//
// Created by steven on 12/2/25.
//
#pragma once
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "token.h"
#include "Value.h"


namespace front::ir {
    class CodegenContext;
}

namespace front::ast {
    enum class BasicType { Int, Void, Float };

    enum class UnaryOp { Positive, Negative, LogicalNot };

    enum class BasicOp {
        Add, Sub, Mul, Div, Mod,
        Lt, Gt, Le, Ge, Eq, Neq,
        And, Or,
    };

    struct Node {
        virtual ~Node() = default;
    };

    struct Expr : Node {
        virtual Value *codegen(ir::CodegenContext &ctx) = 0;

        ~Expr() override = default;
    };

    struct LiteralInt : Expr {
        int value{};

        Value *codegen(ir::CodegenContext &ctx) override;
    };

    struct LiteralFloat : Expr {
        float value{};

        Value *codegen(ir::CodegenContext &ctx) override;
    };

    struct IdentifierExpr : Expr {
        std::string name;

        Value *codegen(ir::CodegenContext &ctx) override;
    };

    struct UnaryExpr : Expr {
        UnaryOp op;
        std::unique_ptr<Expr> operand;

        Value *codegen(ir::CodegenContext &ctx) override;
    };

    struct BinaryExpr : Expr {
        BasicOp op;
        std::unique_ptr<Expr> lhs;
        std::unique_ptr<Expr> rhs;

        Value *codegen(ir::CodegenContext &ctx) override;
    };

    struct CallExpr : Expr {
        std::string callee;
        std::vector<std::unique_ptr<Expr> > args;

        Value *codegen(ir::CodegenContext &ctx) override;
    };

    struct Stmt : Node {
        virtual void codegen(ir::CodegenContext &ctx) = 0;

        ~Stmt() override = default;
    };

    struct EmptyStmt : Stmt {
        void codegen(ir::CodegenContext &ctx) override;
    };

    struct ExprStmt : Stmt {
        std::unique_ptr<Expr> expr;

        void codegen(ir::CodegenContext &ctx) override;
    };

    struct AssignStmt : Stmt {
        std::string target;
        std::unique_ptr<Expr> expr;

        void codegen(ir::CodegenContext &ctx) override;
    };

    struct ReturnStmt : Stmt {
        std::unique_ptr<Expr> value;

        void codegen(ir::CodegenContext &ctx) override;
    };

    struct IfStmt : Stmt {
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Stmt> then_branch;
        std::unique_ptr<Stmt> else_branch;

        void codegen(ir::CodegenContext &ctx) override;
    };


    struct Decl : Node {
        virtual void codegen(ir::CodegenContext &ctx) = 0;

        ~Decl() override = default;
    };

    struct BlockItem {
        enum class Type { Decl, Stmt };

        Type type{Type::Stmt};
        std::unique_ptr<Decl> decl;
        std::unique_ptr<Stmt> stmt;

        static BlockItem make_decl(std::unique_ptr<Decl> decl) {
            return {Type::Decl, std::move(decl), nullptr};
        }

        static BlockItem make_stmt(std::unique_ptr<Stmt> stmt) {
            return {Type::Stmt, nullptr, std::move(stmt)};
        }
    };

    struct BlockStmt : Stmt {
        std::vector<BlockItem> items;

        void codegen(ir::CodegenContext &ctx) override;
    };

    struct VarInit {
        std::string name;
        std::unique_ptr<Expr> value;
    };

    struct VarDecl : Decl {
        bool is_const{false};
        BasicType type{BasicType::Int};
        std::vector<VarInit> items;

        void codegen(ir::CodegenContext &ctx) override;
    };

    struct Param {
        BasicType type{BasicType::Int};
        std::string name;
    };

    struct FuncDef : Node {
        BasicType type{BasicType::Void};
        std::string name;
        std::vector<Param> params;
        std::unique_ptr<BlockStmt> body;

        void codegen(ir::CodegenContext &ctx);
    };

    struct Program : Node {
        std::vector<std::unique_ptr<Decl> > globals;
        std::vector<std::unique_ptr<FuncDef> > functions;

        void codegen(ir::CodegenContext &ctx) const;
    };

    using ExprPtr = std::unique_ptr<Expr>;
    using StmtPtr = std::unique_ptr<Stmt>;
    using DeclPtr = std::unique_ptr<Decl>;
    using BlockPtr = std::unique_ptr<BlockStmt>;
    using FuncPtr = std::unique_ptr<FuncDef>;
    using ProgramPtr = std::unique_ptr<Program>;

    using SemVal = std::variant<
        std::monostate,

        // terminals
        std::string,
        int,
        float,
        BasicType,
        UnaryOp,

        BlockItem,
        std::vector<VarInit>,
        std::vector<Param>,

        // AST Ptr
        ExprPtr,
        StmtPtr,
        DeclPtr,
        BlockPtr,
        FuncPtr,
        ProgramPtr
    >;

    SemVal make_semantic(const Token &token);

    void print_ast(const ProgramPtr &program, std::ostream &os);
}
