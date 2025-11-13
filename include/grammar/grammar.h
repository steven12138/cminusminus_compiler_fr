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

        using RawProduction = std::pair<std::string, std::vector<Symbol> >;

        explicit Grammar(const std::vector<RawProduction> &productions);


        friend std::ostream &operator <<(std::ostream &os, const Grammar &grammar) {
            for (const auto &prod: grammar.productions_) {
                os << prod << "\n";
            }
            return os;
        }

        void init_rules();

        void print_first_set(std::ostream &os) const;

        void print_follow_set(std::ostream &os) const;

    private:
        void add_production(const std::string &name, std::vector<Symbol> body);

        void compute_first_set();

        void compute_follow_set();

        std::vector<Production> productions_;
        std::unordered_map<std::string, std::vector<size_t> > production_map_;
        std::unordered_set<std::string> terminals_;
        std::unordered_set<std::string> non_terminals_;

        std::unordered_map<Symbol, std::unordered_set<Symbol, SymbolHasher>, SymbolHasher> first_set_;
        std::unordered_map<Symbol, std::unordered_set<Symbol, SymbolHasher>, SymbolHasher> follow_set_;
    };
}
