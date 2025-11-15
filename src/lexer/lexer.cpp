#include <stdexcept>

#include "lexer/lexer.h"
#include "lexer/regex.h"

#include "utils/dfa.h"
#include "utils/timer.h"

namespace front::lexer {
    Lexer::Lexer(std::string source)
        : source_(std::move(source)), row{1}, column{1} {
        MESSAGE_TIMER(a, "test");
        const auto nfa = init_rules();
        STOP_TIMER(a);

        MESSAGE_TIMER(b, "DFA Construction");
        dfa = std::make_unique<DFA<Symbol> >(nfa);
        STOP_TIMER(b);

        MESSAGE_TIMER(c, "DFA Minimization");
        dfa->minimalize();
        STOP_TIMER(c);
    }

    Lexer::Lexer() : Lexer("") {
    }


    const std::vector<Token> &Lexer::tokenize() {
        if (source_.empty()) throw std::runtime_error("Lexer::tokenize() source is empty");
        if (!tokens.empty()) return tokens;
        if (dfa->start_state() == -1)
            throw std::runtime_error("DFA has no start state");
        size_t pos = 0;
        while (pos < source_.size()) {
            int state = dfa->start_state();
            size_t cursor = pos;
            int last_accepting_state = -1;
            size_t last_accepting_pos = pos;
            if (state >= 0 && dfa->states()[state].token >= 0) {
                last_accepting_state = state;
                last_accepting_pos = cursor;
            }
            while (cursor < source_.size() && state >= 0) {
                auto c = source_[cursor];
                int next = dfa->transition(state, c);
                if (next < 0) break;
                state = next;
                cursor++;
                if (dfa->states()[state].token >= 0) {
                    last_accepting_state = state;
                    last_accepting_pos = cursor;
                }
            }
            Token token;
            if (last_accepting_state >= 0 && last_accepting_pos > pos) {
                int accept = dfa->states()[last_accepting_state].token;
                token = {
                    std::get<1>(rules[accept]),
                    std::get<2>(rules[accept]),
                    {row, column},
                    source_.substr(pos, last_accepting_pos - pos)
                };
                pos = last_accepting_pos;
            } else {
                token = {
                    TokenType::Invalid,
                    TokenCategory::Invalid,
                    {row, column},
                    {1, source_[pos]}
                };
                pos++;
            }
            tokens.push_back(token);
            advance(token.lexeme);
        }
        optimize();
        return tokens;
    }

    const std::vector<Token> &Lexer::tokenize(const std::string &source) {
        source_ = source;
        tokens.clear();
        row = 1;
        column = 1;
        return tokenize();
    }

    void Lexer::optimize() {
        if (tokens.empty()) return;
        std::vector<Token> optimized_tokens;
        optimized_tokens.reserve(tokens.size());
        for (const auto &token: tokens) {
            if (token.category != TokenCategory::Spacer) {
                optimized_tokens.push_back(token);
            }
        }
        tokens = std::move(optimized_tokens);
    }


    std::unique_ptr<NFA<Symbol> > Lexer::init_rules() {
        rules = {
            {"( |\t)+", TokenType::Spacer, TokenCategory::Spacer},
            {"\r\n", TokenType::Spacer, TokenCategory::Spacer},
            {"\n", TokenType::Spacer, TokenCategory::Spacer},
            {"\r", TokenType::Spacer, TokenCategory::Spacer},

            // keywords
            {"?i:int", TokenType::KwInt, TokenCategory::Keyword},
            {"?i:void", TokenType::KwVoid, TokenCategory::Keyword},
            {"?i:return", TokenType::KwReturn, TokenCategory::Keyword},
            {"?i:main", TokenType::KwMain, TokenCategory::Keyword},
            {"?i:float", TokenType::KwFloat, TokenCategory::Keyword},
            {"?i:if", TokenType::KwIf, TokenCategory::Keyword},
            {"?i:else", TokenType::KwElse, TokenCategory::Keyword},
            {"?i:const", TokenType::KwConst, TokenCategory::Keyword},

            // Operators
            {"==", TokenType::OpEqual, TokenCategory::Operator},
            {"<=", TokenType::OpLessEqual, TokenCategory::Operator},
            {">=", TokenType::OpGreaterEqual, TokenCategory::Operator},
            {"!=", TokenType::OpNotEqual, TokenCategory::Operator},
            {"&&", TokenType::OpAnd, TokenCategory::Operator},
            {"\\|\\|", TokenType::OpOr, TokenCategory::Operator},
            {"\\+", TokenType::OpPlus, TokenCategory::Operator},
            {"-", TokenType::OpMinus, TokenCategory::Operator},
            {"\\*", TokenType::OpMultiply, TokenCategory::Operator},
            {"/", TokenType::OpDivide, TokenCategory::Operator},
            {"%", TokenType::OpMod, TokenCategory::Operator},
            {"=", TokenType::OpAssign, TokenCategory::Operator},
            {">", TokenType::OpGreater, TokenCategory::Operator},
            {"<", TokenType::OpLess, TokenCategory::Operator},

            // Separators
            {"\\(", TokenType::SeLParen, TokenCategory::Separators},
            {"\\)", TokenType::SeRParen, TokenCategory::Separators},
            {"\\{", TokenType::SeLBrace, TokenCategory::Separators},
            {"\\}", TokenType::SeRBrace, TokenCategory::Separators},
            {",", TokenType::SeComma, TokenCategory::Separators},
            {";", TokenType::SeSemicolon, TokenCategory::Separators},

            // Others
            {RULE_FLOAT, TokenType::LiteralFloat, TokenCategory::FloatLiteral},
            {"(" RULE_DIGITS ")+", TokenType::LiteralInt, TokenCategory::IntLiteral},
            {"(" RULE_ID_START ")" "(" RULE_ID_CHAR ")*", TokenType::Identifier, TokenCategory::Identifier},
            {".", TokenType::Invalid, TokenCategory::Invalid},
        };

        std::vector<std::unique_ptr<NFA<Symbol> > > subs{};
        subs.reserve(rules.size());
        for (size_t i = 0; i < rules.size(); ++i) {
            const auto &[pattern, token_type, token_category] = rules[i];
            auto nfa = Regex(pattern).compile(i, i);
            subs.push_back(std::move(nfa));
        }
        auto master_nfa = NFA<Symbol>::union_many(subs);
        return master_nfa;
    }


    void Lexer::advance(const std::string &lexeme) {
        for (int i = 0; lexeme[i] != '\0'; ++i) {
            char c = lexeme[i];
            if (c == '\n') {
                row++;
                column = 1;
            } else if (c == '\r') {
                column = 1;
            } else if (c == '\t') {
                int tab_width = 4;
                int offset = tab_width - ((column - 1) % tab_width);
                column += offset;
            } else {
                column++;
            }
        }
    }
}
