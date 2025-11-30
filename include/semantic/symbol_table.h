#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "../token.h"

namespace front::semantic {

enum class SymbolType {
    Variable,
    Constant,
    Function,
};

enum class DataType {
    Int,
    Float,
    Void,
};

struct Symbol {
    std::string name;
    SymbolType symType;
    DataType dataType;
    bool isConst;
    Location loc;
    
    Symbol(const std::string& n, SymbolType st, DataType dt, bool ic, Location l)
        : name(n), symType(st), dataType(dt), isConst(ic), loc(l) {}
};

class SymbolTable {
public:
    SymbolTable() = default;
    explicit SymbolTable(SymbolTable* parent) : parent_(parent) {}
    
    bool insert(const std::string& name, SymbolType type, DataType dataType, 
                bool isConst, Location loc);
    
    Symbol* lookup(const std::string& name);
    
    SymbolTable* enterScope();
    void exitScope();
    
private:
    std::unordered_map<std::string, Symbol> symbols_;
    SymbolTable* parent_{nullptr};
    std::vector<std::unique_ptr<SymbolTable>> children_;
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer() {
        currentTable_ = &globalTable_;
    }
    
    bool analyze(class front::ast::CompUnitNode* root);
    
    const std::vector<std::string>& getErrors() const { return errors_; }
    
private:
    SymbolTable globalTable_;
    SymbolTable* currentTable_{&globalTable_};
    std::vector<std::string> errors_;
    
    bool analyzeDecl(class front::ast::DeclNode* decl);
    bool analyzeStmt(class front::ast::StmtNode* stmt);
    bool analyzeExpr(class front::ast::ExprNode* expr);
    bool analyzeFuncDef(class front::ast::FuncDefNode* func);
    
    DataType getExprType(class front::ast::ExprNode* expr);
    bool checkTypeCompatible(DataType t1, DataType t2);
    
    void reportError(const std::string& msg, Location loc);
};

} // namespace front::semantic

