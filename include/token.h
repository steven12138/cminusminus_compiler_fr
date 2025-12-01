#pragma once
#include <string>
#include <ostream>

#include "utils/util.h"
#ifdef USE_MAGIC_ENUM
#include <magic_enum/magic_enum.hpp>
#endif


namespace front {
    struct Location {
        int line{1};
        int column{1};
    };

    enum class TokenCategory {
        Keyword, Operator, Separators, Identifier, IntLiteral, FloatLiteral, End, Invalid, FuncDef,
        Spacer,
    };

    enum class TokenType {
        KwInt, KwVoid, KwReturn, KwMain, KwFloat, KwIf, KwElse, KwConst,

        OpEqual, OpLessEqual, OpGreaterEqual, OpNotEqual, OpAnd, OpOr,
        OpPlus, OpMinus, OpMultiply, OpDivide, OpMod, OpAssign, OpGreater, OpLess,

        SepLParen, SepRParen, SepLBrace, SepRBrace, SepComma, SepSemicolon,

        LiteralInt, LiteralFloat,

        Identifier,
        EndOfFile,
        Invalid,
        Spacer,
        KwIntFunc,
        KwFloatFunc,
    };


    struct Token {
        TokenType type{TokenType::Invalid};
        TokenCategory category{TokenCategory::Invalid};
        Location loc{};
        std::string lexeme{};

        Token() = default;

        Token(const TokenType type, const TokenCategory category, const Location loc, std::string lexeme)
            : type(type), category(category), loc(loc), lexeme(std::move(lexeme)) {
        }

        Token(TokenType type, TokenCategory category) : type(type), category(category) {
        }

        bool operator==(const Token &other) const {
            return type == other.type && category == other.category;
        }

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

    struct TokenHash {
        size_t operator()(const Token &token) const {
            const auto h1 = std::hash<int>()(static_cast<int>(token.type));
            const auto h2 = std::hash<int>()(static_cast<int>(token.category));
            return hash_combine(h1, h2);
        }
    };


    inline std::vector<Token> post_process(const std::vector<Token> &tokens) {
        std::vector<Token> adjusted = tokens;
        int brace_depth = 0;
        for (std::size_t i = 0; i < adjusted.size(); ++i) {
            const auto type = adjusted[i].type;
            if (type == TokenType::SepLBrace) {
                ++brace_depth;
            } else if (type == TokenType::SepRBrace) {
                brace_depth = std::max(0, brace_depth - 1);
            }
            if (brace_depth == 0 &&
                (type == TokenType::KwInt || type == TokenType::KwFloat) &&
                i + 2 < adjusted.size() && (adjusted[i + 1].type == TokenType::Identifier ||
                                            adjusted[i + 1].type == TokenType::KwMain) &&
                adjusted[i + 2].type == TokenType::SepLParen) {
                adjusted[i].category = TokenCategory::FuncDef;
                adjusted[i].type =
                        (type == TokenType::KwInt)
                            ? TokenType::KwIntFunc
                            : TokenType::KwFloatFunc;
            }
        }
        return adjusted;
    }
}
