#pragma once

#include <memory>
#include <string>
#include <vector>
#include "../token.h"

namespace front::ast {

enum class ASTNodeType {
    // 表达式
    IntLiteral,
    FloatLiteral,
    Identifier,
    BinaryExpr,
    UnaryExpr,
    CallExpr,
    
    // 语句
    AssignStmt,
    ExprStmt,
    ReturnStmt,
    IfStmt,
    BlockStmt,
    
    // 声明
    VarDecl,
    ConstDecl,
    FuncParam,
    FuncDef,
    
    // 其他
    CompUnit,
};

enum class BinaryOp {
    Add, Sub, Mul, Div, Mod,
    LT, LE, GT, GE, EQ, NE,
    And, Or
};

enum class UnaryOp {
    Plus, Minus, Not
};

struct ASTNode {
    ASTNodeType type;
    Location loc;
    
    ASTNode(ASTNodeType t, Location l) : type(t), loc(l) {}
    virtual ~ASTNode() = default;
};

// 表达式节点
struct ExprNode : public ASTNode {
    ExprNode(ASTNodeType t, Location l) : ASTNode(t, l) {}
};

struct IntLiteralNode : public ExprNode {
    int value;
    IntLiteralNode(int v, Location l) : ExprNode(ASTNodeType::IntLiteral, l), value(v) {}
};

struct FloatLiteralNode : public ExprNode {
    double value;
    FloatLiteralNode(double v, Location l) : ExprNode(ASTNodeType::FloatLiteral, l), value(v) {}
};

struct IdentifierNode : public ExprNode {
    std::string name;
    IdentifierNode(const std::string& n, Location l) 
        : ExprNode(ASTNodeType::Identifier, l), name(n) {}
};

struct BinaryExprNode : public ExprNode {
    BinaryOp op;
    std::unique_ptr<ExprNode> left;
    std::unique_ptr<ExprNode> right;
    
    BinaryExprNode(BinaryOp o, std::unique_ptr<ExprNode> l, 
                   std::unique_ptr<ExprNode> r, Location loc)
        : ExprNode(ASTNodeType::BinaryExpr, loc), op(o), 
          left(std::move(l)), right(std::move(r)) {}
};

struct UnaryExprNode : public ExprNode {
    UnaryOp op;
    std::unique_ptr<ExprNode> operand;
    
    UnaryExprNode(UnaryOp o, std::unique_ptr<ExprNode> opnd, Location loc)
        : ExprNode(ASTNodeType::UnaryExpr, loc), op(o), operand(std::move(opnd)) {}
};

struct CallExprNode : public ExprNode {
    std::string funcName;
    std::vector<std::unique_ptr<ExprNode>> args;
    
    CallExprNode(const std::string& name, Location loc)
        : ExprNode(ASTNodeType::CallExpr, loc), funcName(name) {}
};

// 语句节点
struct StmtNode : public ASTNode {
    StmtNode(ASTNodeType t, Location l) : ASTNode(t, l) {}
};

struct AssignStmtNode : public StmtNode {
    std::string varName;
    std::unique_ptr<ExprNode> value;
    
    AssignStmtNode(const std::string& name, std::unique_ptr<ExprNode> val, Location loc)
        : StmtNode(ASTNodeType::AssignStmt, loc), varName(name), value(std::move(val)) {}
};

struct ExprStmtNode : public StmtNode {
    std::unique_ptr<ExprNode> expr;
    
    ExprStmtNode(std::unique_ptr<ExprNode> e, Location loc)
        : StmtNode(ASTNodeType::ExprStmt, loc), expr(std::move(e)) {}
};

struct ReturnStmtNode : public StmtNode {
    std::unique_ptr<ExprNode> expr;  // 可能为null
    
    ReturnStmtNode(std::unique_ptr<ExprNode> e, Location loc)
        : StmtNode(ASTNodeType::ReturnStmt, loc), expr(std::move(e)) {}
};

struct IfStmtNode : public StmtNode {
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<StmtNode> thenStmt;
    std::unique_ptr<StmtNode> elseStmt;  // 可能为null
    
    IfStmtNode(std::unique_ptr<ExprNode> cond, 
               std::unique_ptr<StmtNode> then,
               std::unique_ptr<StmtNode> els,
               Location loc)
        : StmtNode(ASTNodeType::IfStmt, loc), 
          condition(std::move(cond)), thenStmt(std::move(then)), elseStmt(std::move(els)) {}
};

struct BlockStmtNode : public StmtNode {
    std::vector<std::unique_ptr<StmtNode>> stmts;
    
    BlockStmtNode(Location loc) : StmtNode(ASTNodeType::BlockStmt, loc) {}
};

// 声明节点
struct DeclNode : public ASTNode {
    DeclNode(ASTNodeType t, Location l) : ASTNode(t, l) {}
};

struct VarDeclNode : public DeclNode {
    std::string type;  // "int" or "float"
    std::string name;
    std::unique_ptr<ExprNode> initValue;  // 可能为null
    
    VarDeclNode(const std::string& t, const std::string& n, 
                std::unique_ptr<ExprNode> init, Location loc)
        : DeclNode(ASTNodeType::VarDecl, loc), type(t), name(n), initValue(std::move(init)) {}
};

struct ConstDeclNode : public DeclNode {
    std::string type;
    std::string name;
    std::unique_ptr<ExprNode> value;
    
    ConstDeclNode(const std::string& t, const std::string& n,
                  std::unique_ptr<ExprNode> val, Location loc)
        : DeclNode(ASTNodeType::ConstDecl, loc), type(t), name(n), value(std::move(val)) {}
};

struct FuncParamNode : public ASTNode {
    std::string type;
    std::string name;
    
    FuncParamNode(const std::string& t, const std::string& n, Location loc)
        : ASTNode(ASTNodeType::FuncParam, loc), type(t), name(n) {}
};

struct FuncDefNode : public ASTNode {
    std::string returnType;  // "int", "float", "void"
    std::string name;
    std::vector<std::unique_ptr<FuncParamNode>> params;
    std::unique_ptr<BlockStmtNode> body;
    
    FuncDefNode(const std::string& ret, const std::string& n, Location loc)
        : ASTNode(ASTNodeType::FuncDef, loc), returnType(ret), name(n) {}
};

struct CompUnitNode : public ASTNode {
    std::vector<std::unique_ptr<DeclNode>> decls;
    std::vector<std::unique_ptr<FuncDefNode>> funcs;
    
    CompUnitNode(Location loc) : ASTNode(ASTNodeType::CompUnit, loc) {}
};

} // namespace front::ast

