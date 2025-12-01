//
// Created by steven on 12/2/25.
//
#pragma once
#include <variant>
#include <memory>

#include "token.h"


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

        Location loc{};
    };

    struct Expr : Node {
        virtual ~Expr() = default;
    };

    struct LiteralInt : Node {
        int value{};
    };

    struct LiteralFloat : Node {
        float value{};
    };

    struct IdentifierExpr : Expr {
        std::string name;
    };

    struct UnaryExpr : Expr {
        UnaryOp op;
        std::unique_ptr<Expr> operand;
    };

    struct BinaryExpr : Expr {
        BasicOp op;
        std::unique_ptr<Expr> lhs;
        std::unique_ptr<Expr> rhs;
    };

    struct CallExpr : Expr {
        std::string callee;
        std::vector<std::unique_ptr<Expr> > args;
    };

    struct Stmt : Node {
        virtual ~Stmt() = default;
    };

    struct EmptyStmt : Stmt {
    };

    struct ExprStmt : Stmt {
        std::unique_ptr<Expr> expr;
    };

    struct AssignStmt : Stmt {
        std::string target;
        std::unique_ptr<Expr> expr;
    };

    struct ReturnStmt : Stmt {
        std::unique_ptr<Expr> value;
    };

    struct IfStmt : Stmt {
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Stmt> then_branch;
        std::unique_ptr<Stmt> else_branch;
    };


    struct Decl : Node {
        virtual ~Decl() = default;
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
    };

    struct VarInit {
        std::string name;
        std::unique_ptr<Expr> value;
        Location loc{};
    };

    struct VarDecl : Decl {
        bool is_const{false};
        BasicType type{BasicType::Int};
        std::vector<VarInit> items;
    };

    struct Param {
        BasicType type{BasicType::Int};
        std::string name;
        Location loc{};
    };

    struct FuncDef : Node {
        BasicType type{BasicType::Void};
        std::string name;
        std::vector<Param> params;
        std::unique_ptr<BlockStmt> body;
    };

    struct Program : Node {
        std::vector<std::unique_ptr<Decl> > globals;
        std::vector<std::unique_ptr<FuncDef> > functions;
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
}
