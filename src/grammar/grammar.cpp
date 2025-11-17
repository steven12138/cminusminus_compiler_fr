#include "grammar/grammar.h"

#include <algorithm>
#include <iostream>


namespace front::grammar {
    Grammar::Grammar() {
        init_rules();
        compute_first_set();
        compute_follow_set();
    }

    Grammar::Grammar(const std::string &start,
                     const std::vector<RawProduction> &productions, bool ll1) : start_symbol_(NT(start)) {
        for (const auto &[name, body]: productions) {
            add_production(name, std::move(body));
        }
        if (ll1) normalize_ll1();

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

    void Grammar::normalize_ll1() {
        eliminate_left_recursion();
        left_refactoring();
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

    Symbol prime(const Symbol &sym) {
        Symbol s = sym;
        s.name += "'";
        return s;
    }

    void Grammar::eliminate_left_recursion() {
        // pi->pj Y => pi->d1 Y | d2 Y | ... | dm Y
        // where pj->d1 | d2 | ... | dm is all productions of pj
        int n = static_cast<int>(non_terminals_.size());
        size_t cnt = productions.size();
        std::vector ntv(non_terminals_.begin(), non_terminals_.end());
        for (int i = 0; i < n; i++) {
            const auto &pi = ntv[i];
            for (int j = 0; j < i; j++) {
                const auto &pj = ntv[j];
                // all production pi->*
                const auto prod_ids = production_map_[pi];
                for (const auto &pid: prod_ids) {
                    const auto &prod = productions[pid];
                    if (prod.body.empty() || prod.body[0] != NT(pj) || prod.id == -1u) continue;
                    // pi->pj Y
                    // calculating d1...dm where pj->d1 | d2 | ... | dm
                    const auto &pj_prod_ids = production_map_[pj];
                    std::vector<std::vector<Symbol> > D;
                    for (const auto &pj_pid: pj_prod_ids) {
                        const auto &pj_prod = productions[pj_pid];
                        if (pj_prod.id == -1u) continue;
                        // di
                        const auto &d = pj_prod.body;
                        D.push_back(d);
                    }

                    // replace pi->pj Y with pi->d1 Y | d2 Y | ... | dm Y
                    const auto &Y = std::vector<Symbol>(prod.body.begin() + 1, prod.body.end());
                    for (const auto &d: D) {
                        std::vector<Symbol> new_body;
                        new_body.insert(new_body.end(), d.begin(), d.end());
                        new_body.insert(new_body.end(), Y.begin(), Y.end());
                        productions.push_back({cnt++, NT(pi), std::move(new_body)});
                        production_map_[pi].push_back(cnt - 1);
                    }

                    // mark the old production as invalid
                    productions[pid].id = -1u;
                }
            }
            // eliminate direct left recursion of pi
            const auto &pi_prime = prime(NT(pi));
            // A->A alpha | beta  => A-> beta A' , A'-> alpha A' | ε
            std::vector<Production> new_productions;
            bool exist_left_recursion = false;
            for (const auto &pid: production_map_[pi]) {
                const auto &prod = productions[pid];
                if (prod.id != -1u && !prod.body.empty() && prod.body[0] == NT(pi)) {
                    exist_left_recursion = true;
                    break;
                }
            }
            if (!exist_left_recursion) continue;

            for (const auto &pid: production_map_[pi]) {
                if (productions[pid].id == -1u) continue;
                const auto &prod = productions[pid];
                // A->A alpha
                if (!prod.body.empty() && prod.body[0] == NT(pi)) {
                    // A'-> alpha A'
                    std::vector alpha(prod.body.begin() + 1, prod.body.end());
                    alpha.push_back(pi_prime);
                    new_productions.push_back({cnt++, pi_prime, std::move(alpha)});
                }
                // beta
                else {
                    // A-> beta A'
                    std::vector<Symbol> beta = prod.body;
                    beta.push_back(pi_prime);
                    new_productions.push_back({cnt++, NT(pi), std::move(beta)});
                }
                productions[pid].id = -1u;
            }
            // A'-> ε
            new_productions.push_back({cnt++, pi_prime, {Epsilon()}});
            for (auto &prod: new_productions) {
                productions.push_back(std::move(prod));
                production_map_[pi_prime.name].push_back(prod.id);
            }
            non_terminals_.insert(pi_prime.name);
        }

        // simplify unreachable productions
        std::unordered_set<std::string> reachable;
        reachable.insert(start_symbol_.name);
        size_t size = 0;
        while (size < reachable.size()) {
            size = reachable.size();
            for (const auto &prod: productions) {
                if (prod.id == -1u) continue;
                if (reachable.contains(prod.head.name)) {
                    for (const auto &sym: prod.body) {
                        if (sym.is_non_terminal()) {
                            reachable.insert(sym.name);
                        }
                    }
                }
            }
        }


        // erase invalid productions
        std::vector<Production> valid_productions;
        std::unordered_map<std::string, std::vector<size_t> > new_production_map;
        cnt = 0;
        for (auto &prod: productions) {
            if (prod.id != -1u && reachable.contains(prod.head.name)) {
                prod.id = cnt++;
                new_production_map[prod.head.name].push_back(prod.id);
                valid_productions.push_back(std::move(prod));
            }
        }
        productions = std::move(valid_productions);
        production_map_ = std::move(new_production_map);
    }

    void Grammar::left_refactoring() {
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
