#pragma once
#include <string>


namespace front {
    struct Location {
        int line{1};
        int column{1};
    };

    enum class TokenCategory {
        Keyword, Operator, Seperator, Identifier, IntLiteral, FloatLiteral, End, Invalid,
    };

    enum class TokenType {
        KwInt, KwVoid, KwReturn, KwMain, KwFloat, KwIf, KwElse,

        OpEqual, OpLessEqual, OpGreaterEqual, OpNotEqual, OpAnd, OpOr,
        OpPlus, OpMinus, OpMultiply, OpDivide, OpMod, OpAssign, OpGreater, OpLess,

        SeLParen, SeRParen, SeLBrace, SeRBrace, SeComma, SeSemicolon,

        LiteralInt, LiteralFloat,

        Identifier,
        EndOfFile,
        Invalid,
    };


    struct Token {
        TokenType type{TokenType::Invalid};
        TokenCategory category{TokenCategory::Invalid};
        Location loc{};
        std::string lexeme{};
    };

}
