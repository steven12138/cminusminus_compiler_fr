//
// Created by steven on 11/17/25.
//

#include "grammar/parser_ll.h"

#include <iostream>
#include <ranges>
#include <stack>

#include "grammar/parser.h"


namespace front::grammar {
    LL1Parser::LL1Parser() : LL1Parser(Grammar{true}) {
    }

    LL1Parser::LL1Parser(Grammar grammar) : grammar_(std::move(grammar)) {
        if (grammar_.has_back_tracing(std::cerr)) {
            // warning
            std::cerr << "Warning: The grammar has back-tracing conflicts!" << std::endl;
        }
        compute_action_table();
    }

    void LL1Parser::print_parse_table() {
        std::cout << "LL(1) Parse Table Computed with " << parse_table_.size() << " Entries." << std::endl;
        for (const auto &[k,v]: parse_table_) {
            std::cout << "M[" << k.first.name << ", " << k.second.name << "] = " << v << std::endl;
        }
    }


    std::vector<ParseStep> LL1Parser::parse(const std::vector<Token> &tokens) const {
        const auto &token_map = grammar_.token_to_terminal_;
        std::stack<Symbol> parse_stack;
        parse_stack.push(End());
        parse_stack.push(grammar_.start_symbol_);
        size_t curr = 0;

        std::vector<ParseStep> result;
        result.reserve(tokens.size() * 2);

        while (!parse_stack.empty()) {
            const auto &X = parse_stack.top();
            const auto &a_token = tokens[curr];

            if (!token_map.contains(a_token)) {
                result.emplace_back(X, Symbol::Terminal(a_token.lexeme), Error);
                std::cerr << "Parse Error! at line: "
                        << a_token.loc.line << ", col:" << a_token.loc.column << std::endl;
                if (a_token.category == TokenCategory::Invalid)
                    std::cerr << "Unexpected token:" << a_token.lexeme << std::endl;
                else
                    std::cerr << "Token not in grammar terminal set: " << a_token << std::endl;
                return result;
            }
            const auto &a = token_map.at(a_token);

            if (X.is_end() && a.is_end()) {
                result.emplace_back(X, a, Accept);
                parse_stack.pop();
                break;
            }

            if (X.is_terminal()) {
                if (X == a) {
                    result.emplace_back(X, a, Move);
                    parse_stack.pop();
                    curr++;
                } else {
                    result.emplace_back(X, a, Error);
                    std::cerr << "Parse Error! at line: "
                            << a_token.loc.line << ", col:" << a_token.loc.column << std::endl;
                    std::cerr << "Expected terminal: " << X.name << ", but got: " << a.name << std::endl;
                    return result;
                }
            } else if (X.is_non_terminal()) {
                if (parse_table_.contains({X, a})) {
                    const auto &prod = parse_table_.at({X, a});
                    result.emplace_back(X, a, Reduction);
                    parse_stack.pop();
                    // push alpha in reverse order
                    for (const auto &it: std::ranges::reverse_view(prod.body)) {
                        if (it.is_epsilon()) continue;
                        parse_stack.push(it);
                    }
                } else {
                    result.emplace_back(X, a, Error);
                    std::cerr << "Parse Error! at line: "
                            << a_token.loc.line << ", col:" << a_token.loc.column << std::endl;
                    std::cerr << "No production found for M[" << X.name << ", " << a.name << "]" << std::endl;
                    return result;
                }
            } else if (X.is_epsilon()) {
                parse_stack.pop();
            }
        }
        std::cout << "Parse Successful!" << std::endl;
        return result;
    }

    std::vector<Token> LL1Parser::preprocess_tokens(const std::vector<Token> &tokens) {
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

    void LL1Parser::compute_action_table() {
        // For each production A -> alpha
        // a in FIRST(alpha) and a is not EPS, M[A, a] = A -> alpha
        // EPS in FIRST(alpha), for b in FOLLOW(A), M[A, b] = A -> alpha
        // else error
        for (const auto &prod: grammar_.productions) {
            const auto &A = prod.head;
            const auto &alpha = prod.body;
            const auto &first_alpha = grammar_.first_of_sequence(alpha);


            // For each terminal a in FIRST(alpha) \ {EPS}
            for (const auto &a: first_alpha) {
                if (a.is_epsilon()) continue;
                parse_table_[{A, a}] = prod;
            }

            if (first_alpha.contains(Symbol::Epsilon())) {
                const auto &follow_A = grammar_.follow_set_.at(A);
                for (const auto &b: follow_A) {
                    parse_table_[{A, b}] = prod;
                }
            }
        }
    }
}
