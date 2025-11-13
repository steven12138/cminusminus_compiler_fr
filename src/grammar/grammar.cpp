#include "grammar/grammar.h"


namespace front::grammar {
    Grammar::Grammar() {
        init_rules();
        compute_first_set();
        compute_follow_set();
    }

    Grammar::Grammar(std::string start,
                     const std::vector<RawProduction> &productions) : start_symbol_(NT(std::move(start))) {
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
        Production prod{productions.size(), NT(name), std::move(body)};

        // register new production
        productions.push_back(prod);
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
                    const auto &[_, name, body] = productions[i];

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
                            if (sym.is_epsilon()) continue;
                            auto [it, inserted] = first_set_[k].insert(sym);
                            changed |= inserted;
                        }

                        // If Yi is not nullable, stop
                        if (!first_Yi.contains(Symbol::Epsilon())) {
                            all_nullable = false;
                            break;
                        }
                    }

                    // If all Yi can derive ε, then add ε to FIRST(A)
                    if (all_nullable) {
                        auto [it, inserted] = first_set_[k].insert(Symbol::Epsilon());
                        changed |= inserted;
                    }
                }
            }
        }
    }


    void Grammar::compute_follow_set() {
        follow_set_[start_symbol_].insert(Symbol::End());
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto &[_,N,body]: productions) {
                size_t n = body.size();
                for (size_t i = 0; i < body.size(); i++) {
                    const auto &B = body[i];
                    if (B.is_terminal()) continue;

                    bool all_nullable = true;
                    // beta in body[i+1...n]
                    for (size_t j = i + 1; j < n; j++) {
                        const auto &Y = body[j];
                        const auto &firstY = first_set_[Y];

                        // follow(B) += FIRST(Y) \ {ε}
                        for (const auto &sym: firstY) {
                            if (sym.is_epsilon()) continue;
                            auto [it, inserted] = follow_set_[B].insert(sym);
                            changed |= inserted;
                        }

                        // if FIRST(Y) does not contain ε, stop
                        if (!firstY.contains(Symbol::Epsilon())) {
                            all_nullable = false;
                            break;
                        }
                    }
                    // if beta is nullable, follow(B) += follow(A)
                    if (all_nullable) {
                        const auto &followA = follow_set_[N];
                        for (const auto &sym: followA) {
                            auto [it, inserted] = follow_set_[B].insert(sym);
                            changed |= inserted;
                        }
                    }
                }
            }
        }
    }
}
