#include "grammar/grammar.h"

#include <algorithm>
#include <iostream>
#include <ranges>


namespace front::grammar {
    Grammar::Grammar(const bool ll1) : ll1(ll1) {
        init_rules(ll1);
        if (ll1) normalize_ll1();
        compute_first_set();
        compute_follow_set();
    }

    Grammar::Grammar(const std::string &start,
                     const std::vector<RawProduction> &productions, bool ll1) : start_symbol_(NT(start)), ll1(ll1) {
        for (const auto &[name, body]: productions) {
            add_production(name, body);
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

    std::unordered_set<Symbol, SymbolHash>
    Grammar::first_of_sequence(const std::vector<Symbol> &body) const {
        std::unordered_set<Symbol, SymbolHash> result;

        // empty sequence => FIRST = { ε }
        if (body.empty()) {
            std::cerr << "Error: first_of_sequence called with empty body, returning { ε }\n";
            throw std::runtime_error("first_of_sequence called with empty body");
        }

        bool all_nullable = true;
        for (const auto &Y: body) {
            // explicit ε in the body: it only contributes nullability
            if (Y.is_epsilon()) {
                continue;
            }

            auto itF = first_set_.find(Y);
            if (itF == first_set_.end()) {
                // no FIRST information for Y, treat as non-nullable
                all_nullable = false;
                break;
            }

            bool has_epsilon = false;
            for (const auto &s: itF->second) {
                if (s.is_epsilon()) {
                    has_epsilon = true;
                } else {
                    result.insert(s);
                }
            }

            // stop when Y cannot derive ε
            if (!has_epsilon) {
                all_nullable = false;
                break;
            }
        }

        if (all_nullable) {
            result.insert(Symbol::Epsilon());
        }

        return result;
    }

    bool Grammar::has_back_tracing(std::ostream &os) {
        bool has_conflict = false;

        auto symbol_set_to_names = [](const std::unordered_set<Symbol, SymbolHash> &S) {
            std::vector<std::string> v;
            v.reserve(S.size());
            for (const auto &s: S) {
                v.push_back(s.name);
            }
            return v;
        };


        for (const auto &A_name: non_terminals) {
            // all productions of A
            auto it_ids = production_map.find(A_name);
            if (it_ids == production_map.end()) continue;
            const auto &ids = it_ids->second;

            // filter out invalid productions
            std::vector<size_t> prod_ids;
            prod_ids.reserve(ids.size());
            for (auto pid: ids) {
                if (pid < productions.size() && productions[pid].id != -1u) {
                    prod_ids.push_back(pid);
                }
            }
            if (prod_ids.size() <= 1) continue;

            // cache FIRST(α_i) for each production A -> α_i
            struct ProdFirst {
                size_t pid;
                std::unordered_set<Symbol, SymbolHash> first;
            };
            std::vector<ProdFirst> pfv;
            pfv.reserve(prod_ids.size());
            for (auto pid: prod_ids) {
                const auto &prod = productions[pid];
                pfv.push_back(ProdFirst{
                    pid,
                    first_of_sequence(prod.body) // lookup-based FIRST(α)
                });
            }

            Symbol A_sym = NT(A_name);
            const auto &followA = follow_set_[A_sym];

            // FIRST/FIRST conflicts
            for (size_t i = 0; i < pfv.size(); ++i) {
                for (size_t j = i + 1; j < pfv.size(); ++j) {
                    const auto &fi = pfv[i].first;
                    const auto &fj = pfv[j].first;

                    // compute FIRST(α_i) ∩ FIRST(α_j)  (excluding ε)
                    std::unordered_set<Symbol, SymbolHash> inter;
                    for (const auto &s: fi) {
                        if (s.is_epsilon()) continue;
                        if (fj.find(s) != fj.end()) {
                            inter.insert(s);
                        }
                    }

                    if (!inter.empty()) {
                        has_conflict = true;
                        os << "[FIRST/FIRST CONFLICT] Non-terminal " << A_name << "\n";
                        os << "  Prod1: " << productions[pfv[i].pid] << "\n";
                        os << "  Prod2: " << productions[pfv[j].pid] << "\n";
                        os << "  Shared lookahead: ";
                        auto names = symbol_set_to_names(inter);
                        for (const auto &n: names) os << n << " ";
                        os << "\n\n";
                    }
                }
            }

            // FIRST/FOLLOW conflicts
            for (size_t i = 0; i < pfv.size(); ++i) {
                const auto &fi = pfv[i].first;
                bool has_epsilon = false;
                for (const auto &s: fi) {
                    if (s.is_epsilon()) {
                        has_epsilon = true;
                        break;
                    }
                }
                if (!has_epsilon) continue; // only productions that can derive ε

                for (size_t j = 0; j < pfv.size(); ++j) {
                    if (i == j) continue;
                    const auto &fj = pfv[j].first;

                    std::unordered_set<Symbol, SymbolHash> inter;
                    for (const auto &s: followA) {
                        if (s.is_epsilon()) continue;
                        if (fj.find(s) != fj.end()) {
                            inter.insert(s);
                        }
                    }

                    if (!inter.empty()) {
                        has_conflict = true;
                        os << "[FIRST/FOLLOW CONFLICT] Non-terminal " << A_name << "\n";
                        os << "  Prod(with ε): " << productions[pfv[i].pid] << "\n";
                        os << "  Prod(other):  " << productions[pfv[j].pid] << "\n";
                        os << "  FOLLOW(" << A_name << ") ∩ FIRST(other) = ";
                        for (auto names = symbol_set_to_names(inter);
                             const auto &n: names)
                            os << n << " ";
                        os << "\n\n";
                    }
                }
            }
        }
        return has_conflict;
    }


    void Grammar::add_production(const std::string &name, std::vector<Symbol> body) {
        Production prod{productions.size(), NT(name), std::move(body)};
        if (prod.body.empty()) {
            std::cerr << "Empty production\n" << prod << std::endl;
            throw std::runtime_error("Must use Epsilon() to represent empty production body.");
        }

        // register new production
        productions.push_back(prod);
        production_map[name].push_back(prod.id);
        non_terminals.insert(name);

        // collect terminals and non-terminals
        for (const auto &sym: prod.body) {
            if (sym.is_terminal()) {
                terminals_.insert(sym.name);
            } else if (sym.is_non_terminal()) {
                non_terminals.insert(sym.name);
            }
        }
    }

    Symbol Grammar::prime(const Symbol &sym) const {
        Symbol s = sym;
        do { s.name += "'"; } while (non_terminals.contains(s.name));
        return s;
    }

    void Grammar::eliminate_left_recursion() {
        // pi->pj Y => pi->d1 Y | d2 Y | ... | dm Y
        // where pj->d1 | d2 | ... | dm is all productions of pj
        int n = static_cast<int>(non_terminals.size());
        size_t cnt = productions.size();
        std::vector ntv(non_terminals.begin(), non_terminals.end());
        for (int i = 0; i < n; i++) {
            const auto &pi = ntv[i];
            for (int j = 0; j < i; j++) {
                const auto &pj = ntv[j];
                // all production pi->*
                const auto prod_ids = production_map[pi];
                for (const auto &pid: prod_ids) {
                    const auto &prod = productions[pid];
                    if (prod.body.empty() || prod.body[0] != NT(pj) || prod.id == -1u) continue;
                    // pi->pj Y
                    // calculating d1...dm where pj->d1 | d2 | ... | dm
                    const auto &pj_prod_ids = production_map[pj];
                    std::vector<std::vector<Symbol> > D;
                    for (const auto &pj_pid: pj_prod_ids) {
                        const auto &pj_prod = productions[pj_pid];
                        if (pj_prod.id == -1u) continue;
                        // di
                        const auto &d = pj_prod.body;
                        D.push_back(d);
                    }

                    // replace pi->pj Y with pi->d1 Y | d2 Y | ... | dm Y
                    const auto &Y = std::vector(prod.body.begin() + 1, prod.body.end());
                    for (const auto &d: D) {
                        std::vector<Symbol> new_body;
                        new_body.insert(new_body.end(), d.begin(), d.end());
                        new_body.insert(new_body.end(), Y.begin(), Y.end());
                        productions.push_back({cnt++, NT(pi), std::move(new_body)});
                        production_map[pi].push_back(cnt - 1);
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
            for (const auto &pid: production_map[pi]) {
                const auto &prod = productions[pid];
                if (prod.id != -1u && !prod.body.empty() && prod.body[0] == NT(pi)) {
                    exist_left_recursion = true;
                    break;
                }
            }
            if (!exist_left_recursion) continue;

            for (const auto &pid: production_map[pi]) {
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
                production_map[prod.head.name].push_back(prod.id);
                productions.push_back(std::move(prod));
            }
            non_terminals.insert(pi_prime.name);
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
        std::vector<Production> new_prods;
        std::unordered_map<std::string, std::vector<size_t> > new_map;
        std::unordered_set<std::string> new_nt;
        cnt = 0;
        for (auto &prod: productions) {
            if (prod.id != -1u && reachable.contains(prod.head.name)) {
                prod.id = cnt++;
                new_map[prod.head.name].push_back(prod.id);
                new_nt.insert(prod.head.name);
                new_prods.push_back(std::move(prod));
            }
        }
        productions = std::move(new_prods);
        production_map = std::move(new_map);
        non_terminals = std::move(new_nt);
    }

    void Grammar::eliminate_back_tracing() {
        // A -> By, B->d1 | d2 | ... | dm
        // =>
        // A -> d1 y | d2 y | ... | dm y
        bool changed = true;
        size_t cnt = productions.size();
        while (changed) {
            changed = false;
            size_t p_limit = productions.size();
            for (size_t pid = 0; pid < p_limit; pid++) {
                const auto prod = productions[pid];
                if (prod.id == -1u || (prod.body.size() == 1 && prod.body[0].is_epsilon())) continue;
                const auto &first_sym = prod.body[0];
                if (first_sym.is_terminal()) continue;

                // B->d1 | d2 | ... | dm
                const auto &B_name = first_sym.name;
                const auto &B_prod_ids = production_map[B_name];
                std::vector<std::vector<Symbol> > D;
                for (const auto &B_pid: B_prod_ids) {
                    const auto &B_prod = productions[B_pid];
                    if (B_prod.id == -1u) continue;
                    const auto &d = B_prod.body;
                    D.push_back(d);
                }
                if (D.empty()) continue;

                // do replacement
                changed = true;
                const auto &y = std::vector(prod.body.begin() + 1, prod.body.end());
                for (const auto &d: D) {
                    std::vector<Symbol> new_body;
                    new_body.insert(new_body.end(), d.begin(), d.end());
                    new_body.insert(new_body.end(), y.begin(), y.end());
                    productions.push_back({cnt++, prod.head, std::move(new_body)});
                    production_map[prod.head.name].push_back(cnt - 1);
                }
                // mark old production as invalid
                productions[pid].id = -1u;
                break;
            }
        }

        std::vector<Production> valid_productions;
        std::unordered_map<std::string, std::vector<size_t> > new_production_map;

        cnt = 0;
        for (auto &p: productions) {
            if (p.id == -1u) continue;
            p.id = cnt;
            if (p.head.name.empty()) {
                std::cerr
                        << "Error: Production with empty head name found during back tracing elimination.\n"
                        << p << std::endl;
                throw std::runtime_error("Production with empty head name found during back tracing elimination.");
            }
            new_production_map[p.head.name].push_back(cnt);
            valid_productions.push_back(std::move(p));
            ++cnt;
        }

        productions = std::move(valid_productions);
        production_map = std::move(new_production_map);
    }

    void Grammar::left_refactoring() {
        // A-> d B1| ... | d Bn | G1| ... | Gm
        // =>
        // A-> d A' | G1 | ... | Gm
        // A'-> B1 | ... | Bn

        eliminate_back_tracing();

        size_t cnt = productions.size();
        int n = static_cast<int>(non_terminals.size());
        bool changed = true;
        while (changed) {
            changed = false;
            std::vector ntv(non_terminals.begin(), non_terminals.end());
            for (const auto &A_name: ntv) {
                const auto &all_ids = production_map[A_name];
                std::vector<size_t> prod_ids;
                prod_ids.reserve(all_ids.size());
                for (const auto &pid: all_ids) {
                    if (productions[pid].id != -1u) prod_ids.push_back(pid);
                }
                if (prod_ids.size() < 2) continue;

                // group by first symbol
                std::unordered_map<Symbol, std::vector<size_t>, SymbolHash> groups;
                for (const auto &pid: prod_ids) {
                    const auto &prod = productions[pid];
                    if (prod.body.empty()) continue;
                    groups[prod.body[0]].push_back(pid);
                }

                // for every |group| > 1, do left factoring
                Symbol prefix_sym;
                std::vector<size_t> group_ids;
                for (const auto &[sym, ids]: groups) {
                    if (ids.size() > 1) {
                        prefix_sym = sym;
                        group_ids = ids;
                        break;
                    }
                }
                if (group_ids.empty()) continue; // not exists

                changed = true;
                // do refactor
                Symbol A{NT(A_name)};
                Symbol A_prime = prime(NT(A_name));
                non_terminals.insert(A_prime.name);

                std::vector<Production> new_prods;
                for (const auto &pid: group_ids) {
                    auto &old_prod = productions[pid];
                    if (old_prod.id == -1u) continue;

                    // A -> prefix_sym beta
                    std::vector beta(old_prod.body.begin() + 1, old_prod.body.end());
                    if (beta.empty()) beta.push_back(Epsilon()); // A'-> ε
                    // A'-> beta
                    new_prods.push_back({cnt++, A_prime, std::move(beta)});
                    // mark old production as invalid
                    old_prod.id = -1u;
                }
                // A-> prefix_sym A'
                new_prods.push_back({cnt++, A, {prefix_sym, A_prime}});

                for (const auto &p: new_prods) {
                    production_map[p.head.name].push_back(p.id);
                    productions.push_back(p);
                }
                break;
            }
        }
        // erase invalid productions
        std::vector<Production> valid_productions;
        std::unordered_map<std::string, std::vector<size_t> > new_production_map;
        cnt = 0;
        for (auto &prod: productions) {
            if (prod.id != -1u) {
                prod.id = cnt++;
                new_production_map[prod.head.name].push_back(prod.id);
                valid_productions.push_back(std::move(prod));
            }
        }
        productions = std::move(valid_productions);
        production_map = std::move(new_production_map);
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
            for (const auto &nt: non_terminals) {
                auto k = NT(nt);

                // For each production A → body
                for (const auto &i: production_map[nt]) {
                    const auto &[_, name, body] = productions[i];

                    // Case: A → ε
                    if (body.size() == 1 && body[0].is_epsilon()) {
                        if (!first_set_[k].contains(Symbol::Epsilon())) {
                            first_set_[k].insert(Symbol::Epsilon());
                            changed = true;
                        }
                        continue;
                    }

                    bool all_nullable = true;
                    // A → Y1 Y2 ... Yk
                    for (const auto &Yi: body) {
                        if (Yi.is_epsilon()) break;
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
                    if (B.is_terminal() || B.is_epsilon()) continue;

                    bool all_nullable = true;
                    // beta in body[i+1...n]
                    for (size_t j = i + 1; j < n; j++) {
                        const auto &Y = body[j];
                        if (Y.is_epsilon()) continue;
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
