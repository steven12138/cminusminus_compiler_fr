//
// Created by steven on 12/3/25.
//
#include "ast/ast.h"

namespace front::ast {
    SemVal make_semantic(const Token &token) {
        using enum TokenType;
        switch (token.type) {
            case KwInt:
                return SemVal{BasicType::Int};
            case KwFloat:
                return SemVal{BasicType::Float};
            case KwVoid:
                return SemVal{BasicType::Void};
            case LiteralInt:
                return SemVal{std::stoi(token.lexeme)};
            case LiteralFloat:
                return SemVal{std::stof(token.lexeme)};
            case Identifier:
                return SemVal{token.lexeme};
            case KwIntFunc:
                return SemVal{BasicType::Int};
            case KwFloatFunc:
                return SemVal{BasicType::Float};
            case KwMain:
                // treat main like an identifier so grammar rules expecting Ident work
                return SemVal{token.lexeme};
            default:
                return SemVal{std::monostate{}};
        }
    }

    void indent(std::ostream &os, int depth) {
        for (int i = 0; i < depth; ++i) os << "  ";
    }

    const char *to_string(BasicType type) {
        switch (type) {
            case BasicType::Int: return "int";
            case BasicType::Void: return "void";
            case BasicType::Float: return "float";
        }
        return "unknown";
    }

    const char *to_string(UnaryOp op) {
        switch (op) {
            case UnaryOp::Positive: return "+";
            case UnaryOp::Negative: return "-";
            case UnaryOp::LogicalNot: return "!";
        }
        return "?";
    }

    const char *to_string(BasicOp op) {
        switch (op) {
            case BasicOp::Add: return "+";
            case BasicOp::Sub: return "-";
            case BasicOp::Mul: return "*";
            case BasicOp::Div: return "/";
            case BasicOp::Mod: return "%";
            case BasicOp::Lt: return "<";
            case BasicOp::Gt: return ">";
            case BasicOp::Le: return "<=";
            case BasicOp::Ge: return ">=";
            case BasicOp::Eq: return "==";
            case BasicOp::Neq: return "!=";
            case BasicOp::And: return "&&";
            case BasicOp::Or: return "||";
        }
        return "?";
    }

    void print_expr(const Expr *expr, std::ostream &os, int depth);

    void print_stmt(const Stmt *stmt, std::ostream &os, int depth);

    void print_decl(const Decl *decl, std::ostream &os, int depth);

    void print_var_init(const VarInit &init, std::ostream &os, int depth) {
        indent(os, depth);
        os << init.name;
        if (init.value) {
            os << " =\n";
            print_expr(init.value.get(), os, depth + 1);
        } else {
            os << " <uninitialized>\n";
        }
    }

    void print_block(const BlockStmt *block, std::ostream &os, int depth) {
        indent(os, depth);
        os << "Block\n";
        if (!block) {
            indent(os, depth + 1);
            os << "<null block>\n";
            return;
        }
        for (const auto &item: block->items) {
            indent(os, depth + 1);
            if (item.type == BlockItem::Type::Decl) {
                os << "Decl\n";
                print_decl(item.decl.get(), os, depth + 2);
            } else {
                os << "Stmt\n";
                print_stmt(item.stmt.get(), os, depth + 2);
            }
        }
    }

    void print_expr(const Expr *expr, std::ostream &os, int depth) {
        indent(os, depth);
        if (!expr) {
            os << "<null expr>\n";
            return;
        }
        if (const auto *lit = dynamic_cast<const LiteralInt *>(expr)) {
            os << "LiteralInt " << lit->value << "\n";
        } else if (const auto *lit = dynamic_cast<const LiteralFloat *>(expr)) {
            os << "LiteralFloat " << lit->value << "\n";
        } else if (const auto *id = dynamic_cast<const IdentifierExpr *>(expr)) {
            os << "Identifier " << id->name << "\n";
        } else if (const auto *unary = dynamic_cast<const UnaryExpr *>(expr)) {
            os << "Unary " << to_string(unary->op) << "\n";
            print_expr(unary->operand.get(), os, depth + 1);
        } else if (const auto *binary = dynamic_cast<const BinaryExpr *>(expr)) {
            os << "Binary " << to_string(binary->op) << "\n";
            print_expr(binary->lhs.get(), os, depth + 1);
            print_expr(binary->rhs.get(), os, depth + 1);
        } else if (const auto *call = dynamic_cast<const CallExpr *>(expr)) {
            os << "Call " << call->callee << "\n";
            if (call->args.empty()) {
                indent(os, depth + 1);
                os << "<no args>\n";
            } else {
                for (const auto &arg: call->args) {
                    print_expr(arg.get(), os, depth + 1);
                }
            }
        } else {
            os << "<unknown expr>\n";
        }
    }

    void print_stmt(const Stmt *stmt, std::ostream &os, int depth) {
        if (const auto *block = dynamic_cast<const BlockStmt *>(stmt)) {
            print_block(block, os, depth);
            return;
        }
        indent(os, depth);
        if (!stmt) {
            os << "<null stmt>\n";
            return;
        }
        if (dynamic_cast<const EmptyStmt *>(stmt)) {
            os << "EmptyStmt\n";
        } else if (const auto *expr_stmt = dynamic_cast<const ExprStmt *>(stmt)) {
            os << "ExprStmt\n";
            print_expr(expr_stmt->expr.get(), os, depth + 1);
        } else if (const auto *assign = dynamic_cast<const AssignStmt *>(stmt)) {
            os << "Assign " << assign->target << "\n";
            print_expr(assign->expr.get(), os, depth + 1);
        } else if (const auto *ret = dynamic_cast<const ReturnStmt *>(stmt)) {
            os << "Return\n";
            if (ret->value) {
                print_expr(ret->value.get(), os, depth + 1);
            } else {
                indent(os, depth + 1);
                os << "<void>\n";
            }
        } else if (const auto *ifs = dynamic_cast<const IfStmt *>(stmt)) {
            os << "If\n";
            indent(os, depth + 1);
            os << "Cond\n";
            print_expr(ifs->condition.get(), os, depth + 2);
            indent(os, depth + 1);
            os << "Then\n";
            print_stmt(ifs->then_branch.get(), os, depth + 2);
            if (ifs->else_branch) {
                indent(os, depth + 1);
                os << "Else\n";
                print_stmt(ifs->else_branch.get(), os, depth + 2);
            }
        } else {
            os << "<unknown stmt>\n";
        }
    }

    void print_decl(const Decl *decl, std::ostream &os, int depth) {
        indent(os, depth);
        if (!decl) {
            os << "<null decl>\n";
            return;
        }
        if (const auto *var = dynamic_cast<const VarDecl *>(decl)) {
            os << (var->is_const ? "ConstDecl " : "VarDecl ")
                    << to_string(var->type) << "\n";
            for (const auto &item: var->items) {
                print_var_init(item, os, depth + 1);
            }
        } else {
            os << "<unknown decl>\n";
        }
    }

    void print_params(const std::vector<Param> &params, std::ostream &os, int depth) {
        if (params.empty()) {
            indent(os, depth);
            os << "<none>\n";
            return;
        }
        for (const auto &p: params) {
            indent(os, depth);
            os << to_string(p.type) << " " << p.name << "\n";
        }
    }

    void print_func(const FuncDef &func, std::ostream &os, int depth) {
        indent(os, depth);
        os << "Func " << to_string(func.type) << " " << func.name << "\n";
        indent(os, depth + 1);
        os << "Params\n";
        print_params(func.params, os, depth + 2);
        indent(os, depth + 1);
        os << "Body\n";
        print_block(func.body.get(), os, depth + 2);
    }


    void print_ast(const Program &program, std::ostream &os) {
        os << "Program\n";
        for (const auto &decl: program.globals) {
            indent(os, 1);
            os << "GlobalDecl\n";
            print_decl(decl.get(), os, 2);
        }
        for (const auto &func: program.functions) {
            indent(os, 1);
            os << "Function\n";
            print_func(*func, os, 2);
        }
    }

    void print_ast(const ProgramPtr &program, std::ostream &os) {
        if (!program) {
            os << "<empty AST>\n";
            return;
        }
        print_ast(*program, os);
    }
}
