#include "lexer/dfa.h"

#include <algorithm>
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
            std::vector<int> copy(v);
            std::ranges::sort(copy);
            uint64_t h = 14695981039346656037ull;
            for (const int x: copy) {
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


    void DFA::dfs(int u, std::vector<bool> &reachable) const {
        reachable[u] = 1;
        auto state = st_[u];
        for (const auto &[_, to]: state.edges) {
            if (!reachable[to]) {
                dfs(to, reachable);
            }
        }
    }

    std::vector<Symbol> DFA::collect_alphabet() {
        std::unordered_set<Symbol> alphabet;
        for (const auto &st: st_) {
            for (const auto &[sym, _]: st.edges) {
                alphabet.insert(sym);
            }
        }
        return {alphabet.begin(), alphabet.end()};
    }


    std::vector<int> DFA::find_predecessors(const Group &group, Symbol sym) const {
        std::vector<int> predecessors;
        std::unordered_set<int> in_group{group.states.begin(), group.states.end()};
        for (int i = 0; i < static_cast<int>(st_.size()); i++) {
            const auto &state = st_[i];
            for (const auto &[s, to]: state.edges) {
                if (s == sym && in_group.contains(to)) {
                    predecessors.push_back(i);
                    break;
                }
            }
        }
        return predecessors;
    }


    struct PairHash {
        size_t operator()(const std::pair<int, int> &p) const noexcept {
            uint64_t h = 14695981039346656037ull;
            auto combine = [&](int x) {
                auto y = static_cast<uint64_t>(x);
                h ^= y + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
            };
            combine(p.first);
            combine(p.second);
            return h;
        }
    };

    void DFA::minimalize() {
        int nStates = static_cast<int>(st_.size());

        Partition p{nStates};

        // 1. remove unreachable states
        std::vector reachable(st_.size(), false);
        dfs(start_, reachable);

        // 2. initial partition
        auto alphabet = collect_alphabet();

        std::vector<int> unacceptStates;
        unacceptStates.reserve(nStates);
        std::unordered_map<std::pair<int, int>, int, PairHash> acceptMap;
        std::vector<int> workList;

        for (int i = 0; i < static_cast<int>(st_.size()); i++) {
            if (!reachable[i]) continue;
            // non-accepting state
            if (st_[i].token < 0) {
                unacceptStates.push_back(i);
            }
        }
        if (!unacceptStates.empty()) {
            int gid = p.add_group(std::move(unacceptStates), false, -1, -1);
            workList.push_back(gid);
        }

        // accepting states
        for (int i = 0; i < static_cast<int>(st_.size()); i++) {
            if (!reachable[i]) continue;
            if (st_[i].token >= 0) {
                auto it = acceptMap.find({st_[i].token, st_[i].priority});
                if (it == acceptMap.end()) {
                    const int gid = p.add_group({i}, true, st_[i].token, st_[i].priority);
                    acceptMap[{st_[i].token, st_[i].priority}] = gid;
                    workList.push_back(gid);
                } else {
                    int gid = it->second;
                    p.groups[gid].states.push_back(i);
                    p.state_to_group[i] = gid;
                }
            }
        }

        for (size_t i = 0; i < workList.size(); i++) {
            int splitter = workList[i];
            auto &A = p.groups[splitter];
            for (const auto &sym: alphabet) {
                auto X = find_predecessors(A, sym);
                if (X.empty()) continue;
                for (int k = 0; k < static_cast<int>(p.groups.size()); k++) {
                    if (!p.groups[k].valid) continue;
                    int new_gid = p.split(k, X);
                    if (new_gid >= 0) {
                        workList.push_back(new_gid);
                    }
                }
            }
        }

        DFA minDFA{};
        minDFA.st_.reserve(p.groups.size());
        for (const auto &group: p.groups) {
            if (!group.valid) continue;
            int newState = minDFA.new_state();
            minDFA.st_[newState].token = group.token;
            minDFA.st_[newState].priority = group.priority;
        }
        minDFA.start_ = p.find(start_);

        for (int i = 0; i < static_cast<int>(st_.size()); i++) {
            if (!reachable[i]) continue;
            const int from_gid = p.find(i);
            for (const auto &state = st_[i];
                 const auto &[sym, to]: state.edges) {
                if (!reachable[to]) continue;
                const int to_gid = p.find(to);
                minDFA.add_edge(from_gid, to_gid, sym);
            }
        }
        *this = std::move(minDFA);
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
                os << "  S" << i << "([\"S" << i << "\"]);\n";
            }

            for (const auto &[sym, to]: state.edges) {
                if (sym == EPS) {
                    os << "  S" << i << " -- ε --> S" << to << ";\n";
                } else if (std::isprint(static_cast<unsigned char>(sym))) {
                    os << "  S" << i << " -- '"
                            << static_cast<char>(sym) << "' --> S" << to << ";\n";
                } else {
                    os << "  S" << i << " -- [" << sym << "] --> S" << to << ";\n";
                }
            }
        }

        os << "```\n";
        return os;
    }
}
