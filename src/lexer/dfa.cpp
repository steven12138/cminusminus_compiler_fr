#include "lexer/dfa.h"

#include <iostream>
#include <unordered_map>


namespace front::lexer {
    int DFA::new_state() {
        st_.emplace_back(DFAState{});
        return static_cast<int>(st_.size() - 1);
    }


    struct VectorHash {
        size_t operator()(const std::vector<int> &v) const noexcept {
            // FNV-1a hash
            uint64_t h = 14695981039346656037ull;
            for (const int x: v) {
                const uint64_t y = static_cast<uint64_t>(x);
                // hash combine
                h ^= y + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
            }
            return h;
        }
    };

    DFA::DFA(const std::unique_ptr<NFA> &nfa) {
        if (nfa->num_states() == 0) {
            start_ = 0;
            return;
        }
        st_.reserve(std::max<size_t>(16, nfa->num_states()));

        // 1. start state = epsilon-closure({nfa.start_state()})
        const auto startStates = nfa->epsilon_closure({nfa->start_state()});

        // 2. subset_idx: subset -> dfa_state_idx
        std::unordered_map<std::vector<int>, int, VectorHash> subset_idx;

        std::vector<std::vector<int> > subsets;
        std::vector<int> dfa_states;

        subsets.push_back(startStates);
        start_ = new_state();
        dfa_states.push_back(start_);
        subset_idx.emplace(subsets[0], start_);
        {
            auto [token, priority] = nfa->computing_accept(subsets[0]);
            st_[start_].token = token;
            st_[start_].priority = priority;
        }

        for (size_t i = 0; i < subsets.size(); i++) {
            auto T = subsets[i];
            const int from = dfa_states[i];
            for (auto symbols = nfa->collect_symbols(T);
                 const auto &sym: symbols) {
                // subset = epsilon-closure(move(T, sym))
                auto moveStates = nfa->move(T, sym);
                if (moveStates.empty()) continue;
                auto closure = nfa->epsilon_closure(moveStates);

                // check if closure is already registered
                auto it = subset_idx.find(closure);
                int toState;
                if (it == subset_idx.end()) {
                    // register a new state
                    toState = new_state();
                    subsets.push_back(closure);
                    dfa_states.push_back(toState);
                    subset_idx[closure] = toState;

                    auto [token, priority] = nfa->computing_accept(closure);
                    st_[toState].token = token;
                    st_[toState].priority = priority;
                } else {
                    toState = it->second;
                }
                add_edge(from, toState, sym);
            }
        }
    }

    void DFA::add_edge(int u, int v, Symbol sym) {
        auto &state = st_[u];
        for (auto &edge: state.edges) {
            if (edge.sym == sym) {
                edge.to = v;
                return;
            }
        }

        state.edges.emplace_back(sym, v);
    }

    int DFA::transition(int state, Symbol sym) const {
        int any_target = -1;
        const auto &st = st_[state];
        for (const auto &edge: st.edges) {
            if (edge.sym == sym) {
                return edge.to;
            }
            if (sym != ANY && edge.sym == ANY) {
                any_target = edge.to;
            }
        }
        return any_target;
    }




    void DFA::minimalize() {
    }


    std::ostream &operator<<(std::ostream &os, const DFA &dfa) {
        os << "```mermaid\n";
        os << "graph TD;\n";
        os << "  start((start)) --> S" << dfa.start_ << ";\n";

        for (size_t i = 0; i < dfa.st_.size(); ++i) {
            const auto &state = dfa.st_[i];

            if (state.token >= 0) {
                // S%d([\"S%d (accept rules %d, priority %d)\"]);
                os << "  S" << i << "([\"S" << i
                        << " (accept rules " << state.token
                        << ", priority " << state.priority << ")\"]);\n";
            } else {
                // S%d([\"S%d\"]);
                os << "  S" << i << "([\"S" << i << "\"]);\n";
            }

            for (const auto &[sym, to]: state.edges) {
                if (sym == EPS) {
                    // S%d -- ε --> S%d;
                    os << "  S" << i << " -- ε --> S" << to << ";\n";
                } else if (std::isprint(static_cast<unsigned char>(sym))) {
                    // S%d -- 'c' --> S%d;
                    os << "  S" << i << " -- '"
                            << static_cast<char>(sym) << "' --> S" << to << ";\n";
                } else {
                    // S%d -- [num] --> S%d;
                    os << "  S" << i << " -- [" << sym << "] --> S" << to << ";\n";
                }
            }
        }

        os << "```\n";
        return os;
    }
}
