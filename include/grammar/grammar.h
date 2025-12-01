#pragma once
#include <ostream>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <span>

#include "ast/ast.h"
#include "symbol.h"

namespace front::grammar {
    using ActionFn = std::function<ast::SemVal(std::span<ast::SemVal>)>;

    struct Production {
        size_t id;
        Symbol head;
        std::vector<Symbol> body;
        bool visible{true};
        ActionFn action{nullptr};

        friend std::ostream &operator <<(std::ostream &os, const Production &prod) {
            os << prod.head.name << " -> ";
            if (prod.body.empty()) {
                os << "{" << EPS << "}";
                return os;
            }
            for (const auto &sym: prod.body) {
                os << " " << sym.name;
            }
            return os;
        }
    };

    class Grammar {
    public:
        explicit Grammar(bool ll1 = false);


        using RawProduction = std::pair<std::string, const std::vector<Symbol> &>;


        explicit Grammar(const std::string &start, const std::vector<RawProduction> &productions, bool ll1 = false);

        friend std::ostream &operator <<(std::ostream &os, const Grammar &grammar) {
            for (const auto &prod: grammar.productions) {
                if (prod.id == -1u) continue;
                os << prod << "\n";
            }
            return os;
        }

        void init_rules(bool ll1 = false);

        void init_token_map();

        void print_first_set(std::ostream &os) const;

        void print_follow_set(std::ostream &os) const;

        void normalize_ll1();

        std::unordered_set<Symbol, SymbolHash> first_of_sequence(const std::vector<Symbol> &body) const;

        bool has_back_tracing(std::ostream &os);

        std::vector<Production> productions;
        std::unordered_map<std::string, std::vector<size_t> > production_map;

        std::unordered_set<std::string> terminals_;
        std::unordered_set<std::string> non_terminals;

        Symbol start_symbol_;
        bool ll1{false};

        std::unordered_map<Symbol, std::unordered_set<Symbol, SymbolHash>, SymbolHash> first_set_;
        std::unordered_map<Symbol, std::unordered_set<Symbol, SymbolHash>, SymbolHash> follow_set_;

        std::unordered_map<Token, Symbol, TokenHash> token_to_terminal_;

    private:
        void add_production(const std::string &name, std::vector<Symbol> body,
                            ActionFn action = nullptr, bool visible = true);

        Symbol prime(const Symbol &sym) const;

        void eliminate_left_recursion();

        void eliminate_back_tracing();

        void left_refactoring();

        void compute_first_set();

        void compute_follow_set();
    };
}
