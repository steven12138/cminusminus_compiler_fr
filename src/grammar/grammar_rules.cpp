#include "grammar/grammar.h"


namespace front::grammar {
    void Grammar::init_rules() {
        start_symbol_ = NT("ProgramPrime");
        // Augment grammar: Program' -> Program
        add_production("ProgramPrime", {NT("Program")});

        // Program-> compUnit EOF
        add_production("Program", {NT("CompUnit"), Symbol::End()});

        // compUnit -> ( decl | funcDef)*
        add_production("CompUnit", {});
        add_production("CompUnit", {NT("CompUnitItem"), NT("CompUnit")});
        add_production("CompUnitItem", {NT("Decl")});
        add_production("CompUnitItem", {NT("FuncDef")});

        // decl -> constDecl | varDecl;
        add_production("Decl", {NT("ConstDecl")});
        add_production("Decl", {NT("VarDecl")});

        // constDecl -> 'const' bType constDef (',' constDef)* ';';
        add_production("ConstDecl", {
                           T("const"), NT("BType"), NT("ConstDefList"), T(";")
                       });
        add_production("ConstDefList", {NT("ConstDef")});
        add_production("ConstDefList", {
                           NT("ConstDefList"), T(","), NT("ConstDef")
                       });

        // bType -> 'int' | 'float';
        add_production("BType", {T("int")});
        add_production("BType", {T("float")});

        // constDef -> Ident '=' constInitVal;
        add_production("ConstDef", {T("Ident"), T("="), NT("ConstInitVal")});

        //  constInitVal -> constExp
        add_production("ConstInitVal", {NT("ConstExp")});

        // varDecl -> bType varDef (',' varDef)* ';';
        add_production("VarDecl", {NT("BType"), NT("VarDefList"), T(";")});
        add_production("VarDefList", {NT("VarDef")});
        add_production("VarDefList", {NT("VarDefList"), T(","), NT("VarDef")});

        // varDef -> Ident | Ident '=' initVal ;
        add_production("VarDef", {T("Ident")});
        add_production("VarDef", {T("Ident"), T("="), NT("InitVal")});

        // initVal -> exp;
        add_production("InitVal", {NT("Exp")});

        // funcDef -> funcType Ident '(' (funcFParams)? ')' block;
        add_production("FuncDef", {
                           NT("FuncType"), T("Ident"),
                           T("("), NT("FuncFParamsOpt"), T(")"),
                           NT("Block")
                       });
        add_production("FuncFParamsOpt", {});
        add_production("FuncFParamsOpt", {NT("FuncFParams")});

        // funcType -> 'void' | 'int' ;
        add_production("FuncType", {T("void")});
        add_production("FuncType", {T("int")});

        // funcFParams -> funcFParam (',' funcFParam)*;
        add_production("FuncFParams", {NT("FuncFParam")});
        add_production("FuncFParams", {NT("FuncFParams"), T(","), NT("FuncFParam")});

        // funcFParam -> bType Ident;
        add_production("FuncFParam", {NT("BType"), T("Ident")});

        // block -> '{' (blockItem)* '}';
        add_production("Block", {T("{"), NT("BlockItemList"), T("}")});
        add_production("BlockItemList", {});
        add_production("BlockItemList", {NT("BlockItemList"), NT("BlockItem")});

        // blockItem -> decl | stmt;
        add_production("BlockItem", {NT("Decl")});
        add_production("BlockItem", {NT("Stmt")});

        // stmt ->lVal '=' exp ';'
        //   | (exp)? ';'
        //   | block
        //   | 'if' '(' cond ')' stmt ('else' stmt)?
        //   | 'return' (exp)? ';';
        add_production("Stmt", {NT("LVal"), T("="), NT("Exp"), T(";")});
        add_production("Stmt", {NT("ExpOpt"), T(";")});
        add_production("Stmt", {NT("Block")});
        add_production("Stmt",
                       {
                           T("if"), T("("), NT("Cond"), T(")"),
                           NT("Stmt"), NT("ElseOpt")
                       });
        add_production("ElseOpt", {});
        add_production("ElseOpt", {T("else"), NT("Stmt")});
        add_production("Stmt", {T("return"), NT("ExpOpt"), T(";")});

        // exp -> addExp;
        add_production("Exp", {NT("AddExp")});

        // cond -> lOrExp;
        add_production("Cond", {NT("LOrExp")});

        // lVal -> Ident;
        add_production("LVal", {T("Ident")});

        // primaryExp -> '(' exp ')'
        //     | lVal
        //     | number;
        add_production("PrimaryExp", {T("("), NT("Exp"), T(")")});
        add_production("PrimaryExp", {NT("LVal")});
        add_production("PrimaryExp", {NT("Number")});

        // number -> IntConst | floatConst;
        add_production("Number", {NT("IntConst")});
        add_production("Number", {NT("FloatConst")});

        // unaryExp -> primaryExp
        //     | Ident '(' (funcRParams)? ')'
        //     | unaryOp unaryExp;
        add_production("UnaryExp", {NT("PrimaryExp")});
        add_production("UnaryExp", {
                           T("Ident"),
                           T("("), NT("FuncRParamsOpt"), T(")")
                       });
        add_production("UnaryExp", {NT("UnaryOp"), NT("UnaryExp")});
        add_production("FuncRParamsOpt", {});
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
        add_production("MulExp", {NT("UnaryExp")});
        add_production("MulExp", {NT("MulExp"), T("*"), NT("UnaryExp")});
        add_production("MulExp", {NT("MulExp"), T("/"), NT("UnaryExp")});
        add_production("MulExp", {NT("MulExp"), T("%"), NT("UnaryExp")});

        // addExp -> mulExp | addExp ('+' | '-') mulExp;
        add_production("AddExp", {NT("MulExp")});
        add_production("AddExp", {NT("AddExp"), T("+"), NT("MulExp")});
        add_production("AddExp", {NT("AddExp"), T("-"), NT("MulExp")});

        // relExp -> addExp
        //   | relExp ('<' | '>' | '<=' | '>=') addExp;
        add_production("RelExp", {NT("AddExp")});
        add_production("RelExp", {NT("RelExp"), T("<"), NT("AddExp")});
        add_production("RelExp", {NT("RelExp"), T(">"), NT("AddExp")});
        add_production("RelExp", {NT("RelExp"), T("<="), NT("AddExp")});
        add_production("RelExp", {NT("RelExp"), T(">="), NT("AddExp")});

        // eqExp -> relExp
        //   | eqExp ('==' | '!=') relExp;
        add_production("EqExp", {NT("RelExp")});
        add_production("EqExp", {NT("EqExp"), T("=="), NT("RelExp")});
        add_production("EqExp", {NT("EqExp"), T("!="), NT("RelExp")});

        // lAndExp -> eqExp
        //    | lAndExp '&&' eqExp;
        add_production("LAndExp", {NT("EqExp")});
        add_production("LAndExp", {NT("LAndExp"), T("&&"), NT("EqExp")});

        // lOrExp -> lAndExp
        //     | lOrExp '||' lAndExp;
        add_production("LOrExp", {NT("LAndExp")});
        add_production("LOrExp", {NT("LOrExp"), T("||"), NT("LAndExp")});

        // constExp -> addExp;
        add_production("ConstExp", {NT("AddExp")});

        // IntConst -> [0-9]+ ;
        add_production("IntConst", {T("LiteralInt")});
        // Ident -> [a-zA-Z_][a-zA-Z_0-9]*;
        add_production("Ident", {T("Identifier")});
        // floatConst -> [0-9]+'.'[0-9]+
        add_production("FloatConst", {T("LiteralFloat")});
    }
}
