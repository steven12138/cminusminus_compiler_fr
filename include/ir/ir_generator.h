#pragma once

#include <string>
#include <memory>
#include "../ast/ast.h"

namespace front::ir {

class IRGenerator {
public:
    IRGenerator() = default;
    ~IRGenerator() = default;
    
    // 生成 IR 代码
    std::string generate(ast::CompUnitNode* root);
    
private:
    std::string generateDecl(ast::DeclNode* decl);
    std::string generateStmt(ast::StmtNode* stmt);
    std::string generateExpr(ast::ExprNode* expr);
    std::string generateFuncDef(ast::FuncDefNode* func);
    
    // 辅助函数
    std::string getTypeName(const std::string& type);
    std::string getBinaryOpName(ast::BinaryOp op);
    std::string getUnaryOpName(ast::UnaryOp op);
    
    // 临时变量计数
    int temp_counter_{0};
    std::string newTemp();
};

} // namespace front::ir

