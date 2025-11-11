//
// Created by steven on 11/11/25.
//

#include "../../include/lexer/nfa.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <unordered_set>


namespace front::lexer {
    int NFA::new_state() {
        this->st_.push_back({{}, -1,INT_MAX});
        return static_cast<int>(this->st_.size() - 1);
    }

    void NFA::add_edge(int from, int to, Symbol sym) {
        this->st_[from].edges.push_back({sym, to});
    }

    void NFA::set_accept(int state, int token, int priority) {
        if (auto &st = this->st_[state]; st.priority > priority) {
            st.token = token;
            st.priority = priority;
        }
    }

    std::unique_ptr<NFA> NFA::union_many(const std::vector<std::unique_ptr<NFA> > &subs) {
        std::unique_ptr<NFA> out = std::make_unique<NFA>();

        if (subs.empty()) {
            out->set_start(out->new_state());
            return out;
        }

        int total = 0;
        for (auto &s: subs) total += s->num_states();
        out->st_.reserve(1 + total);

        out->set_start(out->new_state());

        for (auto &sub: subs) {
            if (sub->num_states() == 0 || sub->start_state() < 0) continue;

            const int base = out->num_states();

            // move states
            out->st_.insert(out->st_.end(),
                            std::make_move_iterator(sub->st_.begin()),
                            std::make_move_iterator(sub->st_.end()));

            // re-map edges
            for (int s = 0; s < sub->num_states(); ++s)
                for (auto &[sym, to]: out->st_[base + s].edges)
                    to += base;

            out->add_eps(out->start_state(), base + sub->start_state());
        }
        return out;
    }

    std::vector<int> NFA::epsilon_closure(const std::vector<int> &state) const {
        std::vector<int> stack = state;
        std::unordered_set<int> exists{state.begin(), state.end()};
        std::vector<int> res{state.begin(), state.end()};

        while (!stack.empty()) {
            int st = stack.back();
            stack.pop_back();
            for (const auto &[sym, to]: st_[st].edges) {
                if (sym == EPS && !exists.contains(to)) {
                    exists.insert(to);
                    res.push_back(to);
                    stack.push_back(to);
                }
            }
        }
        std::ranges::sort(res);
        return res;
    }

    std::vector<int> NFA::move(const std::vector<int> &states, Symbol target) {
        std::unordered_set<int> exists{};
        std::vector<int> res;
        for (const auto &st: states) {
            for (const auto &[sym, to]: st_[st].edges) {
                if (sym == target && !exists.contains(to)) {
                    exists.insert(to);
                    res.push_back(to);
                }
            }
        }
        std::ranges::sort(res);
        return res;
    }

    std::vector<Symbol> NFA::collect_symbols(const std::vector<int> &set) const {
        std::unordered_set<Symbol> symbols;
        for (const auto &st: set) {
            for (const auto &[sym, to]: st_[st].edges) {
                if (sym != EPS) {
                    symbols.insert(sym);
                }
            }
        }
        return {symbols.begin(), symbols.end()};
    }

    std::pair<int, int> NFA::computing_accept(const std::vector<int> &state) const {
        int token = -1;
        int priority = std::numeric_limits<int>::max();

        for (const auto &st: state) {
            if (st_[st].token >= 0 && st_[st].priority < priority) {
                priority = st_[st].priority;
                token = st_[st].token;
            }
        }
        return {token, priority};
    }


    std::ostream &operator<<(std::ostream &os, const NFA &nfa) {
        os << "```mermaid\n";
        os << "graph TD;\n";
        os << "  start((start)) --> S" << nfa.start_ << ";\n";
        for (const auto &state: nfa.st_) {
            if (state.token >= 0) {
                os << "  S" << (&state - &nfa.st_[0]) << "([\"S" << (&state - &nfa.st_[0]) <<
                        " (accept rules " <<
                        state.token <<
                        ")\"]);\n";
            } else {
                os << "  S" << (&state - &nfa.st_[0]) << "([\"S" << (&state - &nfa.st_[0]) << "\"]);\n";
            }
            for (const auto &[sym, to]: state.edges) {
                if (sym == EPS) {
                    os << "  S" << (&state - &nfa.st_[0]) << " -- ε --> S" << to << ";\n";
                } else if (isprint(sym)) {
                    os << "  S" << (&state - &nfa.st_[0]) << " -- '" << static_cast<char>(sym) << "' --> S" << to
                            <<
                            ";\n";
                } else {
                    os << "  S" << (&state - &nfa.st_[0]) << " -- [" << sym << "] --> S" << to << ";\n";
                }
            }
        }
        os << "```\n";
        return os;
    }
}
