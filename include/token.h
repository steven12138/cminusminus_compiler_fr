#pragma once
#include <string>
#include <magic_enum/magic_enum.hpp>

namespace front {
    struct Location {
        int line{1};
        int column{1};
    };

    enum class TokenCategory {
        Keyword, Operator, Separators, Identifier, IntLiteral, FloatLiteral, End, Invalid,
        Spacer,
    };

    enum class TokenType {
        KwInt, KwVoid, KwReturn, KwMain, KwFloat, KwIf, KwElse, KwConst,

        OpEqual, OpLessEqual, OpGreaterEqual, OpNotEqual, OpAnd, OpOr,
        OpPlus, OpMinus, OpMultiply, OpDivide, OpMod, OpAssign, OpGreater, OpLess,

        SeLParen, SeRParen, SeLBrace, SeRBrace, SeComma, SeSemicolon,

        LiteralInt, LiteralFloat,

        Identifier,
        EndOfFile,
        Invalid,
        Spacer,
    };


    struct Token {
        TokenType type{TokenType::Invalid};
        TokenCategory category{TokenCategory::Invalid};
        Location loc{};
        std::string lexeme{};

        friend std::ostream &operator<<(std::ostream &os, const Token &token) {
            os << token.lexeme << "\t" << "Token(Type::" << magic_enum::enum_name(token.type)
                    << ", Category::" << magic_enum::enum_name(token.category)
                    << ", Location(" << token.loc.line << "," << token.loc.column << "))";
            return os;
        }
    };
}
