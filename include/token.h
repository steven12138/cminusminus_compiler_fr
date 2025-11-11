#pragma once
#include <string>
#ifdef USE_MAGIC_ENUM
#include <magic_enum/magic_enum.hpp>
#endif


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
#ifdef USE_MAGIC_ENUM
            os << token.lexeme << "\t" << "Token(Type::" << magic_enum::enum_name(token.type)
                    << ", Category::" << magic_enum::enum_name(token.category)
                    << ", Location(" << token.loc.line << "," << token.loc.column << "))";
#else
            os << token.lexeme << "\t" << "Token(Type::" << static_cast<int>(token.type)
                    << ", Category::" << static_cast<int>(token.category)
                    << ", Location(" << token.loc.line << "," << token.loc.column << "))";
#endif

            return os;
        }
    };
}
