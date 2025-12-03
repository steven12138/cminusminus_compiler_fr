#include "grammar/grammar.h"


namespace front::grammar {
    void Grammar::init_rules(bool ll1) {
        start_symbol_ = NT("Program");

        // Program-> compUnit EOF
        add_production("Program", {NT("CompUnit")},
                       nullptr, {{"Progrom", "EOF"}});

        // compUnit -> ( decl | funcDef)*
        add_production("CompUnit", {Epsilon()});
        add_production("CompUnit", {NT("CompUnitList")});
        add_production("CompUnitList", {NT("CompUnitItem")});
        add_production("CompUnitList", {NT("CompUnitList"), NT("CompUnitItem")});
        add_production("CompUnitItem", {NT("Decl")});
        add_production("CompUnitItem", {NT("FuncDef")});


        // decl -> constDecl | varDecl;
        add_production("Decl", {NT("ConstDecl")},
                       nullptr, {{"decl", "constDecl"}});
        add_production("Decl", {NT("VarDecl")},
                       nullptr, {{"decl", "varDecl"}});

        // constDecl -> 'const' bType constDef (',' constDef)* ';';
        add_production("ConstDecl", {
                           T("const"), NT("BType"), NT("ConstDefList"), T(";")
                       }, nullptr, {{"constDecl", "SE_SEMICOLON"}});
        add_production("ConstDefList", {NT("ConstDef")});
        add_production("ConstDefList", {
                           NT("ConstDefList"), T(","), NT("ConstDef")
                       });

        // bType -> 'int' | 'float';
        add_production("BType", {T("int")},
                       nullptr, {{"bType", "int"}});
        add_production("BType", {T("float")},
                       nullptr, {{"bType", "float"}});

        // constDef -> Ident '=' constInitVal;
        add_production("ConstDef", {T("Ident"), T("="), NT("ConstInitVal")},
                       nullptr, {{"constDef", "ConstInitVal"}});

        //  constInitVal -> constExp
        add_production("ConstInitVal", {NT("ConstExp")},
                       nullptr, {{"constInitVal", "constExp"}});

        // varDecl -> bType varDef (',' varDef)* ';';
        add_production("VarDecl", {NT("BType"), NT("VarDefList"), T(";")},
                       nullptr, {{"varDecl", ";"}});
        add_production("VarDefList", {NT("VarDef")});
        add_production("VarDefList", {NT("VarDefList"), T(","), NT("VarDef")});

        // varDef -> Ident | Ident '=' initVal ;
        add_production("VarDef", {T("Ident")},
                       nullptr, {{"varDef", "Ident"}});
        add_production("VarDef", {T("Ident"), T("="), NT("InitVal")},
                       nullptr, {{"varDef", "initVal"}});

        // initVal -> exp;
        add_production("InitVal", {NT("Exp")},
                       nullptr, {{"initVal", "exp"}});

        // funcDef -> funcType Ident '(' (funcFParams)? ')' block;
        add_production("FuncDef", {
                           NT("FuncType"), T("Ident"),
                           T("("), T(")"),
                           NT("Block")
                       }, nullptr, {{"funcDef", "block"}});
        add_production("FuncDef", {
                           NT("FuncType"), T("Ident"),
                           T("("), NT("FuncFParams"), T(")"),
                           NT("Block")
                       }, nullptr, {{"funcDef", "block"}});

        // funcType -> 'void' | 'int' | 'float';
        add_production("FuncType", {T("void")});
        add_production("FuncType", {T("func_int")});
        add_production("FuncType", {T("func_float")});

        // funcFParams -> funcFParam (',' funcFParam)*;
        add_production("FuncFParams", {NT("FuncFParam")});
        add_production("FuncFParams", {NT("FuncFParams"), T(","), NT("FuncFParam")});

        // funcFParam -> bType Ident;
        add_production("FuncFParam", {NT("BType"), T("Ident")});

        // block -> '{' (blockItem)* '}';
        add_production("Block", {T("{"), T("}")},
                       nullptr, {{"block", "}"}});
        add_production("Block", {T("{"), NT("BlockItemList"), T("}")},
                       nullptr, {{"block", "}"}});
        add_production("BlockItemList", {NT("BlockItem")});
        add_production("BlockItemList", {NT("BlockItemList"), NT("BlockItem")});

        // blockItem -> decl | stmt;
        add_production("BlockItem", {NT("Decl")},
                       nullptr, {{"blockItem", "decl"}});
        add_production("BlockItem", {NT("Stmt")},
                       nullptr, {{"blockItem", "stmt"}});

        // stmt ->lVal '=' exp ';'
        //   | (exp)? ';'
        //   | block
        //   | 'if' '(' cond ')' stmt ('else' stmt)?
        //   | 'return' (exp)? ';';
        add_production("Stmt", {NT("LVal"), T("="), NT("Exp"), T(";")},
                       nullptr, {{"stmt", ";"}});
        add_production("Stmt", {NT("Exp"), T(";")},
                       nullptr, {{"stmt", ";"}});
        add_production("Stmt", {T(";")},
                       nullptr, {{"stmt", ";"}});
        add_production("Stmt", {NT("Block")},
                       nullptr, {{"stmt", "block"}});
        add_production("Stmt", {
                           T("if"), T("("), NT("Cond"), T(")"), NT("Stmt")
                       }, nullptr, {{"stmt", "if"}});
        add_production("Stmt", {
                           T("if"), T("("), NT("Cond"), T(")"), NT("Stmt"),
                           T("else"), NT("Stmt")
                       }, nullptr, {{"stmt", "if-else"}});
        add_production("Stmt", {T("return"), NT("Exp"), T(";")},
                       nullptr, {{"stmt", ";"}});
        add_production("Stmt", {T("return"), T(";")},
                       nullptr, {{"stmt", ";"}});

        // exp -> addExp;
        // add_production("Exp", {NT("AddExp")});
        add_production("Exp", {NT("LOrExp")},
                       nullptr, {{"exp", "lOrExp"}});

        // cond -> lOrExp;
        add_production("Cond", {NT("LOrExp")},
                       nullptr, {{"cond", "lOrExp"}});
        // add_production("Cond", {NT("Exp")});

        // lVal -> Ident;
        add_production("LVal", {T("Ident")},
                       nullptr, {{"lVal", "Ident"}});

        // primaryExp -> '(' exp ')'
        //     | lVal
        //     | number;
        add_production("PrimaryExp", {T("("), NT("Exp"), T(")")},
                       nullptr, {{"primaryExp", ")"}});
        add_production("PrimaryExp", {NT("LVal")},
                       nullptr, {{"primaryExp", "lVal"}});
        add_production("PrimaryExp", {NT("Number")},
                       nullptr, {{"primaryExp", "number"}});

        // number -> IntConst | floatConst;
        add_production("Number", {NT("IntConst")},
                       nullptr, {{"number", "IntConst"}});
        add_production("Number", {NT("FloatConst")},
                       nullptr, {{"number", "floatConst"}});

        // unaryExp -> primaryExp
        //     | Ident '(' (funcRParams)? ')'
        //     | unaryOp unaryExp;
        add_production("UnaryExp", {NT("PrimaryExp")},
                       nullptr, {{"unaryExp", "primaryExp"}});
        add_production("UnaryExp", {
                           T("Ident"),
                           T("("), NT("FuncRParamsOpt"), T(")")
                       }, nullptr, {{"unaryExp", "call"}});
        add_production("UnaryExp", {NT("UnaryOp"), NT("UnaryExp")},
                       nullptr, {{"unaryExp", "unaryOp"}});
        add_production("FuncRParamsOpt", {Epsilon()});
        add_production("FuncRParamsOpt", {NT("FuncRParams")});

        // unaryOp -> '+' | '-' | '!';
        add_production("UnaryOp", {T("+")});
        add_production("UnaryOp", {T("-")});
        add_production("UnaryOp", {T("!")});

        // funcRParams -> funcRParam (',' funcRParam)*;
        add_production("FuncRParams", {NT("FuncRParam")});
        add_production("FuncRParams", {
                           NT("FuncRParams"), T(","), NT("FuncRParam")
                       });

        // funcRParam -> exp;
        add_production("FuncRParam", {NT("Exp")});

        // mulExp -> unaryExp
        //   | mulExp ('*' | '/' | '%') unaryExp ;
        add_production("MulExp", {NT("UnaryExp")},
                       nullptr, {{"mulExp", "unaryExp"}});
        add_production("MulExp", {NT("MulExp"), T("*"), NT("UnaryExp")},
                       nullptr, {{"mulExp", "*"}});
        add_production("MulExp", {NT("MulExp"), T("/"), NT("UnaryExp")},
                       nullptr, {{"mulExp", "/"}});
        add_production("MulExp", {NT("MulExp"), T("%"), NT("UnaryExp")},
                       nullptr, {{"mulExp", "%"}});

        // addExp -> mulExp | addExp ('+' | '-') mulExp;
        add_production("AddExp", {NT("MulExp")},
                       nullptr, {{"addExp", "mulExp"}});
        add_production("AddExp", {NT("AddExp"), T("+"), NT("MulExp")},
                       nullptr, {{"addExp", "+"}});
        add_production("AddExp", {NT("AddExp"), T("-"), NT("MulExp")},
                       nullptr, {{"addExp", "-"}});

        // relExp -> addExp
        //   | relExp ('<' | '>' | '<=' | '>=') addExp;
        add_production("RelExp", {NT("AddExp")},
                       nullptr, {{"relExp", "addExp"}});
        add_production("RelExp", {NT("RelExp"), T("<"), NT("AddExp")},
                       nullptr, {{"relExp", "<"}});
        add_production("RelExp", {NT("RelExp"), T(">"), NT("AddExp")},
                       nullptr, {{"relExp", ">"}});
        add_production("RelExp", {NT("RelExp"), T("<="), NT("AddExp")},
                       nullptr, {{"relExp", "<="}});
        add_production("RelExp", {NT("RelExp"), T(">="), NT("AddExp")},
                       nullptr, {{"relExp", ">="}});

        // eqExp -> relExp
        //   | eqExp ('==' | '!=') relExp;
        add_production("EqExp", {NT("RelExp")},
                       nullptr, {{"eqExp", "relExp"}});
        add_production("EqExp", {NT("EqExp"), T("=="), NT("RelExp")},
                       nullptr, {{"eqExp", "=="}});
        add_production("EqExp", {NT("EqExp"), T("!="), NT("RelExp")},
                       nullptr, {{"eqExp", "!="}});

        // lAndExp -> eqExp
        //    | lAndExp '&&' eqExp;
        add_production("LAndExp", {NT("EqExp")},
                       nullptr, {{"lAndExp", "eqExp"}});
        add_production("LAndExp", {NT("LAndExp"), T("&&"), NT("EqExp")},
                       nullptr, {{"lAndExp", "&&"}});

        // lOrExp -> lAndExp
        //     | lOrExp '||' lAndExp;
        add_production("LOrExp", {NT("LAndExp")},
                       nullptr, {{"lOrExp", "lAndExp"}});
        add_production("LOrExp", {NT("LOrExp"), T("||"), NT("LAndExp")},
                       nullptr, {{"lOrExp", "||"}});

        // constExp -> addExp;
        add_production("ConstExp", {NT("AddExp")},
                       nullptr, {{"constExp", "addExp"}});

        // IntConst -> [0-9]+ ;
        add_production("IntConst", {T("LiteralInt")});
        // Ident -> [a-zA-Z_][a-zA-Z_0-9]*;
        add_production("Ident", {T("Identifier")});
        // floatConst -> [0-9]+'.'[0-9]+
        add_production("FloatConst", {T("LiteralFloat")});

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
