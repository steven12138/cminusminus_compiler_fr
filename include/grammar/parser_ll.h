//
// Created by steven on 11/17/25.
//
#pragma once
#include "grammar.h"
#include "parser.h"


namespace front::grammar {
    class LL1Parser {
    public:
        explicit LL1Parser();

        explicit LL1Parser(Grammar grammar);

        void print_parse_table();

        std::vector<ParseStep> parse(const std::vector<Token> &tokens) const;

        std::vector<Token> preprocess_tokens(const std::vector<Token> &tokens) const;

        Grammar grammar_{true};

    private:
        void compute_action_table();
        void handle_dangling_else();

        using ActionHash = PairHash<Symbol, Symbol, SymbolHash, SymbolHash>;

        std::unordered_map<std::pair<Symbol, Symbol>, Production, ActionHash> parse_table_{};
    };
}
