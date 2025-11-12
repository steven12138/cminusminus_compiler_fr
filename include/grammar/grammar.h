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

        friend std::ostream &operator <<(std::ostream &os, const Grammar &grammar) {
            for (const auto &prod: grammar.productions_) {
                os << prod << "\n";
            }
            return os;
        }

    private:
        void add_production(const std::string &name, std::vector<Symbol> body);

        std::vector<Production> productions_;
        std::unordered_map<std::string, std::vector<size_t> > production_map_;
        std::unordered_set<std::string> terminals_;
        std::unordered_set<std::string> non_terminals_;
    };
}
