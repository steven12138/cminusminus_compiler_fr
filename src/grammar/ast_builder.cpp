#include "grammar/parser_slr.h"
#include "ast/ast.h"
#include "token.h"
#include <sstream>

namespace front::grammar {

std::unique_ptr<ast::ASTNode> SLRParser::buildASTNode(
    const Production& prod,
    const std::vector<std::unique_ptr<ast::ASTNode>>& children,
    const std::vector<Token>& tokens,
    size_t token_start, size_t token_end) {
    
    Location loc = token_start < tokens.size() ? tokens[token_start].loc : Location{1, 1};
    
    // 根据产生式头部和产生式ID构建不同的 AST 节点
    const std::string& head = prod.head.name;
    
    // CompUnit 相关
    if (head == "CompUnit") {
        auto node = std::make_unique<ast::CompUnitNode>(loc);
        for (const auto& child : children) {
            if (auto* decl = dynamic_cast<ast::DeclNode*>(child.get())) {
                node->decls.push_back(std::unique_ptr<ast::DeclNode>(
                    dynamic_cast<ast::DeclNode*>(child.release())));
            } else if (auto* func = dynamic_cast<ast::FuncDefNode*>(child.get())) {
                node->funcs.push_back(std::unique_ptr<ast::FuncDefNode>(
                    dynamic_cast<ast::FuncDefNode*>(child.release())));
            }
        }
        return node;
    }
    
    // VarDecl
    if (head == "VarDecl") {
        // VarDecl -> BType VarDefList ';'
        // 这里需要从 tokens 中提取类型和变量名
        if (token_start < tokens.size()) {
            // 查找 BType token
            std::string type = "int";
            std::string name;
            std::unique_ptr<ast::ExprNode> initValue;
            
            // 从 tokens 中提取信息
            for (size_t i = token_start; i < token_end && i < tokens.size(); ++i) {
                if (tokens[i].type == TokenType::KwInt) {
                    type = "int";
                } else if (tokens[i].type == TokenType::KwFloat) {
                    type = "float";
                } else if (tokens[i].type == TokenType::Identifier) {
                    name = tokens[i].lexeme;
                }
            }
            
            // 如果有初始化值，从 children 中获取
            if (children.size() >= 2 && children[1]) {
                if (auto* expr = dynamic_cast<ast::ExprNode*>(children[1].get())) {
                    initValue = std::unique_ptr<ast::ExprNode>(
                        dynamic_cast<ast::ExprNode*>(children[1].release()));
                }
            }
            
            if (!name.empty()) {
                return std::make_unique<ast::VarDeclNode>(type, name, std::move(initValue), loc);
            }
        }
    }
    
    // ConstDecl
    if (head == "ConstDecl") {
        // ConstDecl -> 'const' BType ConstDefList ';'
        if (token_start < tokens.size()) {
            std::string type = "int";
            std::string name;
            std::unique_ptr<ast::ExprNode> value;
            
            // 从 tokens 中提取信息
            for (size_t i = token_start; i < token_end && i < tokens.size(); ++i) {
                if (tokens[i].type == TokenType::KwInt) {
                    type = "int";
                } else if (tokens[i].type == TokenType::KwFloat) {
                    type = "float";
                } else if (tokens[i].type == TokenType::Identifier) {
                    name = tokens[i].lexeme;
                }
            }
            
            // 从 children 中获取值
            if (children.size() >= 2 && children[1]) {
                if (auto* expr = dynamic_cast<ast::ExprNode*>(children[1].get())) {
                    value = std::unique_ptr<ast::ExprNode>(
                        dynamic_cast<ast::ExprNode*>(children[1].release()));
                }
            }
            
            if (!name.empty() && value) {
                return std::make_unique<ast::ConstDeclNode>(type, name, std::move(value), loc);
            }
        }
    }
    
    // FuncDef
    if (head == "FuncDef") {
        // FuncDef -> FuncType Ident '(' FuncFParamsOpt ')' Block
        if (children.size() >= 5) {
            std::string retType = "void";
            std::string funcName;
            std::vector<std::unique_ptr<ast::FuncParamNode>> params;
            std::unique_ptr<ast::BlockStmtNode> body;
            
            // 获取返回类型和函数名
            if (auto* id = dynamic_cast<ast::IdentifierNode*>(children[1].get())) {
                funcName = id->name;
            }
            
            // 获取参数（如果有）
            if (children.size() > 4 && children[3]) {
                // 处理参数列表
            }
            
            // 获取函数体
            if (auto* block = dynamic_cast<ast::BlockStmtNode*>(children[5].get())) {
                body = std::unique_ptr<ast::BlockStmtNode>(
                    dynamic_cast<ast::BlockStmtNode*>(children[5].release()));
            }
            
            auto node = std::make_unique<ast::FuncDefNode>(retType, funcName, loc);
            node->params = std::move(params);
            node->body = std::move(body);
            return node;
        }
    }
    
    // 表达式相关
    if (head == "Exp" || head == "AddExp" || head == "MulExp" || 
        head == "RelExp" || head == "EqExp" || head == "LAndExp" || head == "LOrExp") {
        if (children.size() == 1) {
            return std::move(const_cast<std::vector<std::unique_ptr<ast::ASTNode>>&>(children)[0]);
        } else if (children.size() == 3) {
            // 二元运算
            auto* left = dynamic_cast<ast::ExprNode*>(children[0].get());
            auto* right = dynamic_cast<ast::ExprNode*>(children[2].get());
            if (left && right) {
                // 根据产生式体中的运算符确定 BinaryOp
                ast::BinaryOp op = ast::BinaryOp::Add;
                
                // 从产生式体中查找运算符
                for (const auto& sym : prod.body) {
                    if (sym.is_terminal()) {
                        if (sym.name == "+") op = ast::BinaryOp::Add;
                        else if (sym.name == "-") op = ast::BinaryOp::Sub;
                        else if (sym.name == "*") op = ast::BinaryOp::Mul;
                        else if (sym.name == "/") op = ast::BinaryOp::Div;
                        else if (sym.name == "%") op = ast::BinaryOp::Mod;
                        else if (sym.name == "<") op = ast::BinaryOp::LT;
                        else if (sym.name == "<=") op = ast::BinaryOp::LE;
                        else if (sym.name == ">") op = ast::BinaryOp::GT;
                        else if (sym.name == ">=") op = ast::BinaryOp::GE;
                        else if (sym.name == "==") op = ast::BinaryOp::EQ;
                        else if (sym.name == "!=") op = ast::BinaryOp::NE;
                        else if (sym.name == "&&") op = ast::BinaryOp::And;
                        else if (sym.name == "||") op = ast::BinaryOp::Or;
                    }
                }
                
                return std::make_unique<ast::BinaryExprNode>(
                    op, 
                    std::unique_ptr<ast::ExprNode>(dynamic_cast<ast::ExprNode*>(children[0].release())),
                    std::unique_ptr<ast::ExprNode>(dynamic_cast<ast::ExprNode*>(children[2].release())),
                    loc);
            }
        }
    }
    
    // UnaryExp
    if (head == "UnaryExp" && children.size() == 2) {
        auto* operand = dynamic_cast<ast::ExprNode*>(children[1].get());
        if (operand) {
            ast::UnaryOp op = ast::UnaryOp::Plus;
            // 从产生式体中查找一元运算符
            for (const auto& sym : prod.body) {
                if (sym.is_terminal()) {
                    if (sym.name == "+") op = ast::UnaryOp::Plus;
                    else if (sym.name == "-") op = ast::UnaryOp::Minus;
                    else if (sym.name == "!") op = ast::UnaryOp::Not;
                }
            }
            return std::make_unique<ast::UnaryExprNode>(
                op,
                std::unique_ptr<ast::ExprNode>(dynamic_cast<ast::ExprNode*>(children[1].release())),
                loc);
        }
    }
    
    // IntLiteral
    if (head == "IntConst") {
        if (token_start < tokens.size()) {
            const Token& token = tokens[token_start];
            if (token.type == TokenType::LiteralInt) {
                int value = std::stoi(token.lexeme);
                return std::make_unique<ast::IntLiteralNode>(value, loc);
            }
        }
    }
    
    // FloatLiteral
    if (head == "FloatConst") {
        if (token_start < tokens.size()) {
            const Token& token = tokens[token_start];
            if (token.type == TokenType::LiteralFloat) {
                double value = std::stod(token.lexeme);
                return std::make_unique<ast::FloatLiteralNode>(value, loc);
            }
        }
    }
    
    // Identifier
    if (head == "Ident") {
        if (token_start < tokens.size()) {
            const Token& token = tokens[token_start];
            if (token.type == TokenType::Identifier) {
                return std::make_unique<ast::IdentifierNode>(token.lexeme, loc);
            }
        }
    }
    
    // 如果有子节点，返回第一个
    if (!children.empty()) {
        return std::move(const_cast<std::vector<std::unique_ptr<ast::ASTNode>>&>(children)[0]);
    }
    
    return nullptr;
}

} // namespace front::grammar

