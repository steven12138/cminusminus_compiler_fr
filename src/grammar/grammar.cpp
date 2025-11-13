#include "grammar/grammar.h"


namespace front::grammar {
    Grammar::Grammar() {
        init_rules();
        compute_first_set();
        compute_follow_set();
    }

    Grammar::Grammar(const std::vector<RawProduction> &productions) {
        for (const auto &prod: productions) {
            add_production(prod.first, prod.second);
        }
        compute_first_set();
        compute_follow_set();
    }


    void Grammar::print_first_set(std::ostream &os) const {
        for (const auto &[sym, firsts]: first_set_) {
            os << "FIRST(" << sym.name << ") = { ";
            bool first = true;
            for (const auto &[type, name]: firsts) {
                if (!first) {
                    os << ",\t";
                }
                os << name;
                first = false;
            }
            os << " }\n";
        }
    }

    void Grammar::print_follow_set(std::ostream &os) const {
        for (const auto &[sym, follows]: follow_set_) {
            os << "FOLLOW(" << sym.name << ") = { ";
            bool first = true;
            for (const auto &[type, name]: follows) {
                if (!first) {
                    os << ", \t";
                }
                os << name;
                first = false;
            }
            os << " }\n";
        }
    }


    void Grammar::add_production(const std::string &name, std::vector<Symbol> body) {
        Production prod{productions_.size(), NT(name), std::move(body)};

        // register new production
        productions_.push_back(prod);
        production_map_[name].push_back(prod.id);
        non_terminals_.insert(name);

        // collect terminals and non-terminals
        for (const auto &sym: prod.body) {
            if (sym.is_terminal()) {
                terminals_.insert(sym.name);
            } else if (sym.is_non_terminal()) {
                non_terminals_.insert(sym.name);
            }
        }
    }

    void Grammar::compute_first_set() {
        // 1. for terminals, FIRST(a) = {a}
        for (const auto &terminal: terminals_) {
            auto t = T(terminal);
            first_set_[t].insert(t);
        }

        bool changed = true;
        while (changed) {
            changed = false;

            // For each non-terminal A
            for (const auto &nt: non_terminals_) {
                auto k = NT(nt);

                // For each production A → body
                for (const auto &i: production_map_[nt]) {
                    const auto &[_, name, body] = productions_[i];

                    // Case: A → ε
                    if (body.empty()) {
                        if (!first_set_[k].contains(Symbol::Epsilon())) {
                            first_set_[k].insert(Symbol::Epsilon());
                            changed = true;
                        }
                        continue;
                    }

                    bool all_nullable = true;
                    // A → Y1 Y2 ... Yk
                    for (const auto &Yi: body) {
                        const auto &first_Yi = first_set_[Yi];

                        // Add FIRST(Yi) \ {ε} to FIRST(A)
                        for (const auto &sym: first_Yi) {
                            if (sym == Symbol::Epsilon()) continue;

                            if (auto [it, inserted]
                                        = first_set_[k].insert(sym);
                                inserted) {
                                changed = true;
                            }
                        }

                        // If Yi is not nullable, stop
                        if (!first_Yi.contains(Symbol::Epsilon())) {
                            all_nullable = false;
                            break;
                        }
                    }

                    // If all Yi can derive ε, then add ε to FIRST(A)
                    if (all_nullable) {
                        if (!first_set_[k].contains(Symbol::Epsilon())) {
                            first_set_[k].insert(Symbol::Epsilon());
                            changed = true;
                        }
                    }
                }
            }
        }
    }


    void Grammar::compute_follow_set() {
    }
}
