#include "ir/ir_generator.h"
#include "ast/ast.h"
#include <sstream>
#include <iostream>

namespace front::ir {

std::string IRGenerator::generate(ast::CompUnitNode* root) {
    std::ostringstream oss;
    
    // 生成全局声明
    for (auto& decl : root->decls) {
        oss << generateDecl(decl.get());
    }
    
    // 生成函数定义
    for (auto& func : root->funcs) {
        oss << generateFuncDef(func.get());
    }
    
    return oss.str();
}

std::string IRGenerator::generateDecl(ast::DeclNode* decl) {
    std::ostringstream oss;
    
    if (auto* varDecl = dynamic_cast<ast::VarDeclNode*>(decl)) {
        std::string type = getTypeName(varDecl->type);
        oss << "declare " << type << " @" << varDecl->name;
        if (varDecl->initValue) {
            std::string value = generateExpr(varDecl->initValue.get());
            oss << " = " << value;
        }
        oss << "\n";
    } else if (auto* constDecl = dynamic_cast<ast::ConstDeclNode*>(decl)) {
        std::string type = getTypeName(constDecl->type);
        std::string value = generateExpr(constDecl->value.get());
        oss << "declare " << type << " @" << constDecl->name 
             << " = " << value << "\n";
    }
    
    return oss.str();
}

std::string IRGenerator::generateStmt(ast::StmtNode* stmt) {
    std::ostringstream oss;
    
    if (auto* assign = dynamic_cast<ast::AssignStmtNode*>(stmt)) {
        std::string value = generateExpr(assign->value.get());
        oss << "  @" << assign->varName << " = " << value << "\n";
    } else if (auto* ret = dynamic_cast<ast::ReturnStmtNode*>(stmt)) {
        if (ret->expr) {
            std::string value = generateExpr(ret->expr.get());
            oss << "  ret " << value << "\n";
        } else {
            oss << "  ret void\n";
        }
    } else if (auto* ifStmt = dynamic_cast<ast::IfStmtNode*>(stmt)) {
        std::string cond = generateExpr(ifStmt->condition.get());
        std::string thenLabel = newTemp();
        std::string elseLabel = newTemp();
        std::string endLabel = newTemp();
        
        oss << "  br " << cond << ", label %" << thenLabel 
            << ", label %" << elseLabel << "\n";
        oss << "\n" << thenLabel << ":\n";
        oss << generateStmt(ifStmt->thenStmt.get());
        oss << "  br label %" << endLabel << "\n";
        
        if (ifStmt->elseStmt) {
            oss << "\n" << elseLabel << ":\n";
            oss << generateStmt(ifStmt->elseStmt.get());
            oss << "  br label %" << endLabel << "\n";
        } else {
            oss << "\n" << elseLabel << ":\n";
            oss << "  br label %" << endLabel << "\n";
        }
        
        oss << "\n" << endLabel << ":\n";
    } else if (auto* block = dynamic_cast<ast::BlockStmtNode*>(stmt)) {
        for (auto& s : block->stmts) {
            oss << generateStmt(s.get());
        }
    } else if (auto* exprStmt = dynamic_cast<ast::ExprStmtNode*>(stmt)) {
        generateExpr(exprStmt->expr.get());  // 表达式语句，值被丢弃
    }
    
    return oss.str();
}

std::string IRGenerator::generateExpr(ast::ExprNode* expr) {
    std::ostringstream oss;
    
    if (auto* intLit = dynamic_cast<ast::IntLiteralNode*>(expr)) {
        return std::to_string(intLit->value);
    } else if (auto* floatLit = dynamic_cast<ast::FloatLiteralNode*>(expr)) {
        return std::to_string(floatLit->value);
    } else if (auto* id = dynamic_cast<ast::IdentifierNode*>(expr)) {
        return "@" + id->name;
    } else if (auto* binary = dynamic_cast<ast::BinaryExprNode*>(expr)) {
        std::string left = generateExpr(binary->left.get());
        std::string right = generateExpr(binary->right.get());
        std::string op = getBinaryOpName(binary->op);
        std::string temp = newTemp();
        
        oss << "  %" << temp << " = " << op << " " << left 
            << ", " << right << "\n";
        return "%" + temp;
    } else if (auto* unary = dynamic_cast<ast::UnaryExprNode*>(expr)) {
        std::string operand = generateExpr(unary->operand.get());
        std::string op = getUnaryOpName(unary->op);
        std::string temp = newTemp();
        
        oss << "  %" << temp << " = " << op << " " << operand << "\n";
        return "%" + temp;
    } else if (auto* call = dynamic_cast<ast::CallExprNode*>(expr)) {
        std::string temp = newTemp();
        oss << "  %" << temp << " = call @" << call->funcName << "(";
        for (size_t i = 0; i < call->args.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << generateExpr(call->args[i].get());
        }
        oss << ")\n";
        return "%" + temp;
    }
    
    return "";
}

std::string IRGenerator::generateFuncDef(ast::FuncDefNode* func) {
    std::ostringstream oss;
    
    std::string retType = getTypeName(func->returnType);
    oss << "define " << retType << " @" << func->name << "(";
    
    // 参数
    for (size_t i = 0; i < func->params.size(); ++i) {
        if (i > 0) oss << ", ";
        std::string paramType = getTypeName(func->params[i]->type);
        oss << paramType << " %" << func->params[i]->name;
    }
    oss << ") {\n";
    
    // 函数体
    if (func->body) {
        oss << generateStmt(func->body.get());
    }
    
    oss << "}\n\n";
    return oss.str();
}

std::string IRGenerator::getTypeName(const std::string& type) {
    if (type == "int") return "i32";
    if (type == "float") return "float";
    if (type == "void") return "void";
    return "i32";  // 默认
}

std::string IRGenerator::getBinaryOpName(ast::BinaryOp op) {
    switch (op) {
        case ast::BinaryOp::Add: return "add";
        case ast::BinaryOp::Sub: return "sub";
        case ast::BinaryOp::Mul: return "mul";
        case ast::BinaryOp::Div: return "sdiv";
        case ast::BinaryOp::Mod: return "srem";
        case ast::BinaryOp::LT: return "icmp slt";
        case ast::BinaryOp::LE: return "icmp sle";
        case ast::BinaryOp::GT: return "icmp sgt";
        case ast::BinaryOp::GE: return "icmp sge";
        case ast::BinaryOp::EQ: return "icmp eq";
        case ast::BinaryOp::NE: return "icmp ne";
        case ast::BinaryOp::And: return "and";
        case ast::BinaryOp::Or: return "or";
        default: return "add";
    }
}

std::string IRGenerator::getUnaryOpName(ast::UnaryOp op) {
    switch (op) {
        case ast::UnaryOp::Plus: return "add";
        case ast::UnaryOp::Minus: return "sub";
        case ast::UnaryOp::Not: return "xor";
        default: return "add";
    }
}

std::string IRGenerator::newTemp() {
    return "t" + std::to_string(temp_counter_++);
}

} // namespace front::ir

