#pragma once
#include "symbol.h"
#include <ostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>


namespace front::grammar {
    struct Production {
        size_t id;
        Symbol head;
        std::vector<Symbol> body;

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
        explicit Grammar();


        using RawProduction = std::pair<std::string, const std::vector<Symbol> &>;

        // explicit Grammar(const std::string& start, const std::vector<RawProduction> &productions);

        explicit Grammar(const std::string &start, const std::vector<RawProduction> &productions, bool ll1 = false);

        friend std::ostream &operator <<(std::ostream &os, const Grammar &grammar) {
            for (const auto &prod: grammar.productions) {
                if (prod.id == -1u) continue;
                os << prod << "\n";
            }
            return os;
        }

        void init_rules();

        void print_first_set(std::ostream &os) const;

        void print_follow_set(std::ostream &os) const;

        void normalize_ll1();

        std::vector<Production> productions;
        std::unordered_map<std::string, std::vector<size_t> > production_map_;

    private:
        void add_production(const std::string &name, std::vector<Symbol> body);

        void eliminate_left_recursion();

        void left_refactoring();

        void compute_first_set();

        void compute_follow_set();

        std::unordered_set<std::string> terminals_;
        std::unordered_set<std::string> non_terminals_;

        Symbol start_symbol_;

        std::unordered_map<Symbol, std::unordered_set<Symbol, SymbolHasher>, SymbolHasher> first_set_;
        std::unordered_map<Symbol, std::unordered_set<Symbol, SymbolHasher>, SymbolHasher> follow_set_;
    };
}
