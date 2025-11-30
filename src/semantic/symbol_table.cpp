#include "semantic/symbol_table.h"
#include "ast/ast.h"
#include <iostream>

namespace front::semantic {

bool SymbolTable::insert(const std::string& name, SymbolType type, 
                         DataType dataType, bool isConst, Location loc) {
    if (symbols_.find(name) != symbols_.end()) {
        return false;  // 重复定义
    }
    symbols_.emplace(name, Symbol(name, type, dataType, isConst, loc));
    return true;
}

Symbol* SymbolTable::lookup(const std::string& name) {
    SymbolTable* table = this;
    while (table) {
        auto it = table->symbols_.find(name);
        if (it != table->symbols_.end()) {
            return &it->second;
        }
        table = table->parent_;
    }
    return nullptr;
}

SymbolTable* SymbolTable::enterScope() {
    auto child = std::make_unique<SymbolTable>(this);
    SymbolTable* ptr = child.get();
    children_.push_back(std::move(child));
    return ptr;
}

void SymbolTable::exitScope() {
    // 由父表管理子表生命周期
}

bool SemanticAnalyzer::analyze(ast::CompUnitNode* root) {
    errors_.clear();
    
    // 分析所有声明
    for (auto& decl : root->decls) {
        if (!analyzeDecl(decl.get())) {
            return false;
        }
    }
    
    // 分析所有函数
    for (auto& func : root->funcs) {
        if (!analyzeFuncDef(func.get())) {
            return false;
        }
    }
    
    return errors_.empty();
}

bool SemanticAnalyzer::analyzeDecl(ast::DeclNode* decl) {
    if (auto* varDecl = dynamic_cast<ast::VarDeclNode*>(decl)) {
        DataType dt = (varDecl->type == "int") ? DataType::Int : DataType::Float;
        if (!currentTable_->insert(varDecl->name, SymbolType::Variable, dt, false, varDecl->loc)) {
            reportError("重复定义的变量: " + varDecl->name, varDecl->loc);
            return false;
        }
        if (varDecl->initValue) {
            if (!analyzeExpr(varDecl->initValue.get())) {
                return false;
            }
            DataType exprType = getExprType(varDecl->initValue.get());
            if (!checkTypeCompatible(dt, exprType)) {
                reportError("类型不匹配", varDecl->loc);
                return false;
            }
        }
    } else if (auto* constDecl = dynamic_cast<ast::ConstDeclNode*>(decl)) {
        DataType dt = (constDecl->type == "int") ? DataType::Int : DataType::Float;
        if (!currentTable_->insert(constDecl->name, SymbolType::Constant, dt, true, constDecl->loc)) {
            reportError("重复定义的常量: " + constDecl->name, constDecl->loc);
            return false;
        }
        if (!analyzeExpr(constDecl->value.get())) {
            return false;
        }
        DataType exprType = getExprType(constDecl->value.get());
        if (!checkTypeCompatible(dt, exprType)) {
            reportError("常量类型不匹配", constDecl->loc);
            return false;
        }
    }
    return true;
}

bool SemanticAnalyzer::analyzeStmt(ast::StmtNode* stmt) {
    if (auto* assign = dynamic_cast<ast::AssignStmtNode*>(stmt)) {
        Symbol* sym = currentTable_->lookup(assign->varName);
        if (!sym) {
            reportError("未定义的变量: " + assign->varName, assign->loc);
            return false;
        }
        if (sym->isConst) {
            reportError("不能给常量赋值: " + assign->varName, assign->loc);
            return false;
        }
        if (!analyzeExpr(assign->value.get())) {
            return false;
        }
        DataType exprType = getExprType(assign->value.get());
        if (!checkTypeCompatible(sym->dataType, exprType)) {
            reportError("赋值类型不匹配", assign->loc);
            return false;
        }
    } else if (auto* ret = dynamic_cast<ast::ReturnStmtNode*>(stmt)) {
        if (ret->expr && !analyzeExpr(ret->expr.get())) {
            return false;
        }
    } else if (auto* ifStmt = dynamic_cast<ast::IfStmtNode*>(stmt)) {
        if (!analyzeExpr(ifStmt->condition.get())) {
            return false;
        }
        if (!analyzeStmt(ifStmt->thenStmt.get())) {
            return false;
        }
        if (ifStmt->elseStmt && !analyzeStmt(ifStmt->elseStmt.get())) {
            return false;
        }
    } else if (auto* block = dynamic_cast<ast::BlockStmtNode*>(stmt)) {
        auto* oldTable = currentTable_;
        currentTable_ = currentTable_->enterScope();
        for (auto& s : block->stmts) {
            if (!analyzeStmt(s.get())) {
                currentTable_ = oldTable;
                return false;
            }
        }
        currentTable_ = oldTable;
    } else if (auto* exprStmt = dynamic_cast<ast::ExprStmtNode*>(stmt)) {
        if (!analyzeExpr(exprStmt->expr.get())) {
            return false;
        }
    }
    return true;
}

bool SemanticAnalyzer::analyzeExpr(ast::ExprNode* expr) {
    if (auto* id = dynamic_cast<ast::IdentifierNode*>(expr)) {
        Symbol* sym = currentTable_->lookup(id->name);
        if (!sym) {
            reportError("未定义的标识符: " + id->name, id->loc);
            return false;
        }
    } else if (auto* binary = dynamic_cast<ast::BinaryExprNode*>(expr)) {
        if (!analyzeExpr(binary->left.get()) || !analyzeExpr(binary->right.get())) {
            return false;
        }
    } else if (auto* unary = dynamic_cast<ast::UnaryExprNode*>(expr)) {
        if (!analyzeExpr(unary->operand.get())) {
            return false;
        }
    } else if (auto* call = dynamic_cast<ast::CallExprNode*>(expr)) {
        Symbol* sym = currentTable_->lookup(call->funcName);
        if (!sym || sym->symType != SymbolType::Function) {
            reportError("未定义的函数: " + call->funcName, call->loc);
            return false;
        }
        for (auto& arg : call->args) {
            if (!analyzeExpr(arg.get())) {
                return false;
            }
        }
    }
    return true;
}

bool SemanticAnalyzer::analyzeFuncDef(ast::FuncDefNode* func) {
    DataType retType = (func->returnType == "int") ? DataType::Int : 
                      (func->returnType == "float") ? DataType::Float : DataType::Void;
    
    if (!currentTable_->insert(func->name, SymbolType::Function, retType, false, func->loc)) {
        reportError("重复定义的函数: " + func->name, func->loc);
        return false;
    }
    
    auto* oldTable = currentTable_;
    currentTable_ = currentTable_->enterScope();
    
    // 添加参数到符号表
    for (auto& param : func->params) {
        DataType dt = (param->type == "int") ? DataType::Int : DataType::Float;
        currentTable_->insert(param->name, SymbolType::Variable, dt, false, param->loc);
    }
    
    // 分析函数体
    if (func->body && !analyzeStmt(func->body.get())) {
        currentTable_ = oldTable;
        return false;
    }
    
    currentTable_ = oldTable;
    return true;
}

DataType SemanticAnalyzer::getExprType(ast::ExprNode* expr) {
    if (auto* intLit = dynamic_cast<ast::IntLiteralNode*>(expr)) {
        return DataType::Int;
    } else if (auto* floatLit = dynamic_cast<ast::FloatLiteralNode*>(expr)) {
        return DataType::Float;
    } else if (auto* id = dynamic_cast<ast::IdentifierNode*>(expr)) {
        Symbol* sym = currentTable_->lookup(id->name);
        return sym ? sym->dataType : DataType::Int;  // 默认返回Int
    } else if (auto* binary = dynamic_cast<ast::BinaryExprNode*>(expr)) {
        DataType leftType = getExprType(binary->left.get());
        DataType rightType = getExprType(binary->right.get());
        // 如果有一个是Float，结果就是Float
        return (leftType == DataType::Float || rightType == DataType::Float) 
               ? DataType::Float : DataType::Int;
    } else if (auto* unary = dynamic_cast<ast::UnaryExprNode*>(expr)) {
        return getExprType(unary->operand.get());
    } else if (auto* call = dynamic_cast<ast::CallExprNode*>(expr)) {
        Symbol* sym = currentTable_->lookup(call->funcName);
        return sym ? sym->dataType : DataType::Int;
    }
    return DataType::Int;
}

bool SemanticAnalyzer::checkTypeCompatible(DataType t1, DataType t2) {
    if (t1 == t2) return true;
    // Int和Float可以互相转换
    if ((t1 == DataType::Int && t2 == DataType::Float) ||
        (t1 == DataType::Float && t2 == DataType::Int)) {
        return true;
    }
    return false;
}

void SemanticAnalyzer::reportError(const std::string& msg, Location loc) {
    errors_.push_back("[" + std::to_string(loc.line) + ":" + 
                     std::to_string(loc.column) + "] " + msg);
    std::cerr << errors_.back() << std::endl;
}

} // namespace front::semantic

