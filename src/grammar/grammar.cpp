#include "grammar/grammar.h"


namespace front::grammar {
    void Grammar::add_production(const std::string &name, std::vector<Symbol> body) {
        Production prod{productions_.size(), NT(name), std::move(body)};

        // register new production
        productions_.push_back(prod);
        production_map_[name].push_back(prod.id);

        // collect terminals and non-terminals
        for (const auto &sym: prod.body) {
            if (sym.is_terminal()) {
                terminals_.insert(sym.name);
            } else if (sym.is_non_terminal()) {
                non_terminals_.insert(sym.name);
            }
        }
    }
}
