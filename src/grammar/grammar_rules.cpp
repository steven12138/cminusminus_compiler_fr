#include "ast/ast_builder.h"
#include "grammar/grammar.h"


namespace front::grammar {
    void Grammar::init_rules(bool) {
        start_symbol_ = NT("Program");
        using namespace ast;

        // Program-> compUnit EOF
        add_production("Program", {NT("CompUnit")},
                       build_single_forward, {{"Program", "EOF"}});

        // compUnit -> ( decl | funcDef)*
        add_production("CompUnit", {Epsilon()}, [](std::vector<SemVal> &) -> SemVal {
            return ProgramPtr(std::make_unique<Program>());
        });
        add_production("CompUnit", {NT("CompUnitList")}, build_single_forward);
        add_production("CompUnitList", {NT("CompUnitItem")}, build_comp_unit_list_item);
        add_production("CompUnitList", {NT("CompUnitList"), NT("CompUnitItem")},
                       build_comp_unit_list_append);
        add_production("CompUnitItem", {NT("Decl")}, build_single_forward);
        add_production("CompUnitItem", {NT("FuncDef")}, build_single_forward);


        // decl -> constDecl | varDecl;
        add_production("Decl", {NT("ConstDecl")},
                       build_single_forward, {{"decl", "constDecl"}});
        add_production("Decl", {NT("VarDecl")},
                       build_single_forward, {{"decl", "varDecl"}});

        // constDecl -> 'const' bType constDef (',' constDef)* ';';
        add_production("ConstDecl", {
                           T("const"), NT("BType"), NT("ConstDefList"), T(";")
                       }, build_const_decl, {{"constDecl", "SE_SEMICOLON"}});
        add_production("ConstDefList", {NT("ConstDef")}, build_def_list_item);
        add_production("ConstDefList", {
                           NT("ConstDefList"), T(","), NT("ConstDef")
                       }, build_def_list_append);

        // bType -> 'int' | 'float';
        add_production("BType", {T("int")},
                       build_type_int, {{"bType", "int"}});
        add_production("BType", {T("float")},
                       build_type_float, {{"bType", "float"}});

        // constDef -> Ident '=' constInitVal;
        add_production("ConstDef", {T("Ident"), T("="), NT("ConstInitVal")},
                       build_const_def, {{"constDef", "ConstInitVal"}});

        //  constInitVal -> constExp
        add_production("ConstInitVal", {NT("ConstExp")},
                       build_single_forward, {{"constInitVal", "constExp"}});

        // varDecl -> bType varDef (',' varDef)* ';';
        add_production("VarDecl", {NT("BType"), NT("VarDefList"), T(";")},
                       build_var_decl, {{"varDecl", ";"}});
        add_production("VarDefList", {NT("VarDef")}, build_def_list_item);
        add_production("VarDefList", {NT("VarDefList"), T(","), NT("VarDef")},
                       build_def_list_append);

        // varDef -> Ident | Ident '=' initVal ;
        add_production("VarDef", {T("Ident")},
                       build_var_def_uninit, {{"varDef", "Ident"}});
        add_production("VarDef", {T("Ident"), T("="), NT("InitVal")},
                       build_var_def_init, {{"varDef", "initVal"}});

        // initVal -> exp;
        add_production("InitVal", {NT("Exp")},
                       build_single_forward, {{"initVal", "exp"}});

        // funcDef -> funcType Ident '(' (funcFParams)? ')' block;
        add_production("FuncDef", {
                           NT("FuncType"), T("Ident"),
                           T("("), T(")"),
                           NT("Block")
                       }, build_func_def_no_params, {{"funcDef", "block"}});
        add_production("FuncDef", {
                           NT("FuncType"), T("Ident"),
                           T("("), NT("FuncFParams"), T(")"),
                           NT("Block")
                       }, build_func_def, {{"funcDef", "block"}});

        // funcType -> 'void' | 'int' | 'float';
        add_production("FuncType", {T("void")}, build_type_void);
        add_production("FuncType", {T("func_int")}, build_type_int);
        add_production("FuncType", {T("func_float")}, build_type_float);

        // funcFParams -> funcFParam (',' funcFParam)*;
        add_production("FuncFParams", {NT("FuncFParam")}, build_func_fparams_item);
        add_production("FuncFParams", {NT("FuncFParams"), T(","), NT("FuncFParam")},
                       build_func_fparams_append);

        // funcFParam -> bType Ident;
        add_production("FuncFParam", {NT("BType"), T("Ident")}, build_func_fparam);

        // block -> '{' (blockItem)* '}';
        add_production("Block", {T("{"), T("}")},
                       build_block_empty, {{"block", "}"}});
        add_production("Block", {T("{"), NT("BlockItemList"), T("}")},
                       build_block, {{"block", "}"}});

        add_production("BlockItemList", {NT("BlockItem")}, build_block_item_list_item);
        add_production("BlockItemList", {NT("BlockItemList"), NT("BlockItem")},
                       build_block_item_list_append);

        // blockItem -> decl | stmt;
        add_production("BlockItem", {NT("Decl")},
                       build_block_item_decl, {{"blockItem", "decl"}});
        add_production("BlockItem", {NT("Stmt")},
                       build_block_item_stmt, {{"blockItem", "stmt"}});

        // stmt rules
        add_production("Stmt", {NT("LVal"), T("="), NT("Exp"), T(";")},
                       build_stmt_assign, {{"stmt", ";"}});
        add_production("Stmt", {NT("Exp"), T(";")},
                       build_stmt_exp, {{"stmt", ";"}});
        add_production("Stmt", {T(";")},
                       build_stmt_empty, {{"stmt", ";"}});
        add_production("Stmt", {NT("Block")},
                       build_single_forward, {{"stmt", "block"}});
        add_production("Stmt", {
                           T("if"), T("("), NT("Cond"), T(")"), NT("Stmt")
                       }, build_stmt_if, {{"stmt", "if"}});
        add_production("Stmt", {
                           T("if"), T("("), NT("Cond"), T(")"), NT("Stmt"),
                           T("else"), NT("Stmt")
                       }, build_stmt_if_else, {{"stmt", "if-else"}});
        add_production("Stmt", {T("return"), NT("Exp"), T(";")},
                       build_stmt_return, {{"stmt", ";"}});
        add_production("Stmt", {T("return"), T(";")},
                       build_stmt_return_void, {{"stmt", ";"}});

        // exp -> addExp;
        // add_production("Exp", {NT("AddExp")});
        add_production("Exp", {NT("LOrExp")},
                       build_single_forward, {{"exp", "lOrExp"}});

        // cond -> lOrExp;
        add_production("Cond", {NT("LOrExp")},
                       build_single_forward, {{"cond", "lOrExp"}});
        // add_production("Cond", {NT("Exp")});

        // lVal -> Ident;
        add_production("LVal", {T("Ident")},
                       build_lval_ident, {{"lVal", "Ident"}});

        // primaryExp -> '(' exp ')'
        add_production("PrimaryExp", {T("("), NT("Exp"), T(")")},
                       [](std::vector<SemVal> &rhs) { return std::move(rhs[1]); },
                       {{"primaryExp", ")"}});

        add_production("PrimaryExp", {NT("LVal")},
                       build_exp_lval, {{"primaryExp", "lVal"}});
        add_production("PrimaryExp", {NT("Number")},
                       build_single_forward, {{"primaryExp", "number"}});

        // number -> IntConst | floatConst;
        add_production("Number", {NT("IntConst")},
                       build_exp_int, {{"number", "IntConst"}});
        add_production("Number", {NT("FloatConst")},
                       build_exp_float, {{"number", "floatConst"}});

        // unaryExp -> primaryExp
        add_production("UnaryExp", {NT("PrimaryExp")},
                       build_single_forward, {{"unaryExp", "primaryExp"}});

        // Function Call
        // Ident ( Opt ) -> build_exp_call
        add_production("UnaryExp", {
                           T("Ident"),
                           T("("), NT("FuncRParamsOpt"), T(")")
                       }, build_exp_call, {{"unaryExp", "call"}});

        // unaryOp unaryExp
        add_production("UnaryExp", {NT("UnaryOp"), NT("UnaryExp")},
                       build_unary_exp, {{"unaryExp", "unaryOp"}});

        // FuncRParamsOpt
        add_production("FuncRParamsOpt", {Epsilon()},
                       [](std::vector<SemVal> &) { return SemVal{}; });
        add_production("FuncRParamsOpt", {NT("FuncRParams")}, build_single_forward);

        // unaryOp -> '+' | '-' | '!';
        add_production("UnaryOp", {T("+")}, build_unary_op_positive);
        add_production("UnaryOp", {T("-")}, build_unary_op_negative);
        add_production("UnaryOp", {T("!")}, build_unary_op_not);

        // funcRParams -> funcRParam (',' funcRParam)*;
        add_production("FuncRParams", {NT("FuncRParam")}, build_func_rparams_item);
        add_production("FuncRParams", {
                           NT("FuncRParams"), T(","), NT("FuncRParam")
                       }, build_func_rparams_append);

        // funcRParam -> exp;
        add_production("FuncRParam", {NT("Exp")}, build_single_forward);

        // --- Binary Expressions (Using shared builder logic) ---

        // mulExp -> unaryExp
        //   | mulExp ('*' | '/' | '%') unaryExp ;
        add_production("MulExp", {NT("UnaryExp")},
                       build_single_forward, {{"mulExp", "unaryExp"}});
        add_production("MulExp", {NT("MulExp"), T("*"), NT("UnaryExp")},
                       build_binary_mul, {{"mulExp", "*"}});
        add_production("MulExp", {NT("MulExp"), T("/"), NT("UnaryExp")},
                       build_binary_div, {{"mulExp", "/"}});
        add_production("MulExp", {NT("MulExp"), T("%"), NT("UnaryExp")},
                       build_binary_mod, {{"mulExp", "%"}});

        // addExp -> mulExp | addExp ('+' | '-') mulExp;
        add_production("AddExp", {NT("MulExp")},
                       build_single_forward, {{"addExp", "mulExp"}});
        add_production("AddExp", {NT("AddExp"), T("+"), NT("MulExp")},
                       build_binary_add, {{"addExp", "+"}});
        add_production("AddExp", {NT("AddExp"), T("-"), NT("MulExp")},
                       build_binary_sub, {{"addExp", "-"}});

        // relExp -> addExp
        //   | relExp ('<' | '>' | '<=' | '>=') addExp;
        add_production("RelExp", {NT("AddExp")},
                       build_single_forward, {{"relExp", "addExp"}});
        add_production("RelExp", {NT("RelExp"), T("<"), NT("AddExp")},
                       build_binary_lt, {{"relExp", "<"}});
        add_production("RelExp", {NT("RelExp"), T(">"), NT("AddExp")},
                       build_binary_gt, {{"relExp", ">"}});
        add_production("RelExp", {NT("RelExp"), T("<="), NT("AddExp")},
                       build_binary_le, {{"relExp", "<="}});
        add_production("RelExp", {NT("RelExp"), T(">="), NT("AddExp")},
                       build_binary_ge, {{"relExp", ">="}});

        // eqExp -> relExp
        //   | eqExp ('==' | '!=') relExp;
        add_production("EqExp", {NT("RelExp")},
                       build_single_forward, {{"eqExp", "relExp"}});
        add_production("EqExp", {NT("EqExp"), T("=="), NT("RelExp")},
                       build_binary_eq, {{"eqExp", "=="}});
        add_production("EqExp", {NT("EqExp"), T("!="), NT("RelExp")},
                       build_binary_neq, {{"eqExp", "!="}});

        // lAndExp -> eqExp
        //    | lAndExp '&&' eqExp;
        add_production("LAndExp", {NT("EqExp")},
                       build_single_forward, {{"lAndExp", "eqExp"}});
        add_production("LAndExp", {NT("LAndExp"), T("&&"), NT("EqExp")},
                       build_binary_and, {{"lAndExp", "&&"}});

        // lOrExp -> lAndExp
        //     | lOrExp '||' lAndExp;
        add_production("LOrExp", {NT("LAndExp")},
                       build_single_forward, {{"lOrExp", "lAndExp"}});
        add_production("LOrExp", {NT("LOrExp"), T("||"), NT("LAndExp")},
                       build_binary_or, {{"lOrExp", "||"}});

        // constExp -> addExp;
        add_production("ConstExp", {NT("AddExp")},
                       build_single_forward, {{"constExp", "addExp"}});

        // IntConst -> [0-9]+ ;
        // Note: build_exp_int handles the conversion from raw token int to ExprPtr
        add_production("IntConst", {T("LiteralInt")}, build_single_forward);
        // Ident -> [a-zA-Z_][a-zA-Z_0-9]*;
        add_production("Ident", {T("Identifier")}, build_single_forward);
        // floatConst -> [0-9]+'.'[0-9]+
        add_production("FloatConst", {T("LiteralFloat")}, build_single_forward);

        init_token_map();
    }

    void Grammar::init_token_map() {
        // Keywords
        token_to_terminal_[{TokenType::KwInt, TokenCategory::Keyword}] = T("int");
        token_to_terminal_[{TokenType::KwVoid, TokenCategory::Keyword}] = T("void");
        token_to_terminal_[{TokenType::KwReturn, TokenCategory::Keyword}] = T("return");
        token_to_terminal_[{TokenType::KwMain, TokenCategory::Keyword}] = T("Ident");
        token_to_terminal_[{TokenType::KwFloat, TokenCategory::Keyword}] = T("float");
        token_to_terminal_[{TokenType::KwIf, TokenCategory::Keyword}] = T("if");
        token_to_terminal_[{TokenType::KwElse, TokenCategory::Keyword}] = T("else");
        token_to_terminal_[{TokenType::KwConst, TokenCategory::Keyword}] = T("const");

        // Operators
        token_to_terminal_[{TokenType::OpEqual, TokenCategory::Operator}] = T("==");
        token_to_terminal_[{TokenType::OpLessEqual, TokenCategory::Operator}] = T("<=");
        token_to_terminal_[{TokenType::OpGreaterEqual, TokenCategory::Operator}] = T(">=");
        token_to_terminal_[{TokenType::OpNotEqual, TokenCategory::Operator}] = T("!=");
        token_to_terminal_[{TokenType::OpAnd, TokenCategory::Operator}] = T("&&");
        token_to_terminal_[{TokenType::OpOr, TokenCategory::Operator}] = T("||");
        token_to_terminal_[{TokenType::OpPlus, TokenCategory::Operator}] = T("+");
        token_to_terminal_[{TokenType::OpMinus, TokenCategory::Operator}] = T("-");
        token_to_terminal_[{TokenType::OpMultiply, TokenCategory::Operator}] = T("*");
        token_to_terminal_[{TokenType::OpDivide, TokenCategory::Operator}] = T("/");
        token_to_terminal_[{TokenType::OpMod, TokenCategory::Operator}] = T("%");
        token_to_terminal_[{TokenType::OpAssign, TokenCategory::Operator}] = T("=");
        token_to_terminal_[{TokenType::OpGreater, TokenCategory::Operator}] = T(">");
        token_to_terminal_[{TokenType::OpLess, TokenCategory::Operator}] = T("<");

        // Separators
        token_to_terminal_[{TokenType::SepLParen, TokenCategory::Separators}] = T("(");
        token_to_terminal_[{TokenType::SepRParen, TokenCategory::Separators}] = T(")");
        token_to_terminal_[{TokenType::SepLBrace, TokenCategory::Separators}] = T("{");
        token_to_terminal_[{TokenType::SepRBrace, TokenCategory::Separators}] = T("}");
        token_to_terminal_[{TokenType::SepComma, TokenCategory::Separators}] = T(",");
        token_to_terminal_[{TokenType::SepSemicolon, TokenCategory::Separators}] = T(";");

        // Literals
        token_to_terminal_[{TokenType::LiteralInt, TokenCategory::IntLiteral}] = T("LiteralInt");
        token_to_terminal_[{TokenType::LiteralFloat, TokenCategory::FloatLiteral}] = T("LiteralFloat");

        // Identifier
        token_to_terminal_[{TokenType::Identifier, TokenCategory::Identifier}] = T("Ident");
        // End of File
        token_to_terminal_[{TokenType::EndOfFile, TokenCategory::End}] = Symbol::End();

        // Func Decl
        token_to_terminal_[{TokenType::KwIntFunc, TokenCategory::FuncDef}] = T("func_int");
        token_to_terminal_[{TokenType::KwFloatFunc, TokenCategory::FuncDef}] = T("func_float");
    }
}
