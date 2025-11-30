#include "../../include/utils/dfa.h"

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <unordered_set>


namespace front {
    template<typename T, typename V>
    int DFA<T, V>::new_state() {
        st_.emplace_back(DFAState<T, V>{});
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


    template<typename T, typename V>
    DFA<T, V>::DFA(const std::unique_ptr<NFA<T> > &nfa) {
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
        std::ranges::sort(subsets[0]);
        subset_idx.emplace(subsets[0], start_);
        {
            auto [token, priority] = nfa->computing_accept(subsets[0]);
            st_[start_].token = token;
            st_[start_].priority = priority;
        }

        for (size_t i = 0; i < subsets.size(); i++) {
            auto t = subsets[i];
            const int from = dfa_states[i];
            for (auto symbols = nfa->collect_symbols(t);
                 const auto &sym: symbols) {
                // subset = epsilon-closure(move(T, sym))
                auto moveStates = nfa->move(t, sym);
                if (moveStates.empty()) continue;
                auto closure = nfa->epsilon_closure(moveStates);
                std::ranges::sort(closure);
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


    template<typename T, typename V>
    void DFA<T, V>::add_edge(int u, int v, T sym) {
        auto &state = st_[u];
        for (auto &edge: state.edges) {
            if (edge.sym == sym) {
                edge.to = v;
                return;
            }
        }

        state.edges.emplace_back(sym, v);
    }


    template<typename T, typename V>
    int DFA<T, V>::transition(int state, T sym) const {
        int any_target = -1;
        const auto &st = st_[state];
        for (const auto &edge: st.edges) {
            if (edge.sym == sym) {
                return edge.to;
            }
            if (sym != lexer::ANY && edge.sym == lexer::ANY) {
                any_target = edge.to;
            }
        }
        return any_target;
    }


    template<typename T, typename V>
    void DFA<T, V>::dfs(int u, std::vector<bool> &reachable) const {
        reachable[u] = 1;
        for (const auto &state = st_[u];
             const auto &[_, to]: state.edges) {
            if (!reachable[to]) {
                dfs(to, reachable);
            }
        }
    }


    template<typename T, typename V>
    std::vector<T> DFA<T, V>::collect_alphabet() {
        std::unordered_set<T> alphabet;
        for (const auto &st: st_) {
            for (const auto &[sym, _]: st.edges) {
                alphabet.insert(sym);
            }
        }
        return {alphabet.begin(), alphabet.end()};
    }


    template<typename T, typename V>
    std::vector<int> DFA<T, V>::find_predecessors(const Group &group, T sym, const RevGraph &rev) const {
        std::vector<int> predecessors;
        predecessors.reserve(group.states.size());
        std::unordered_set<int> seen;

        for (int q: group.states) {
            if (q < 0 || q >= static_cast<int>(rev.size())) continue;
            const auto &inEdges = rev[q];
            for (const auto &e: inEdges) {
                if (e.sym == sym && !seen.contains(e.to)) {
                    seen.insert(e.to);
                    predecessors.push_back(e.to);
                }
            }
        }
        return predecessors;
    }

    template<typename T, typename V>
    DFA<T, V>::RevGraph DFA<T, V>::build_reverse_edges(const std::vector<bool> &reachable) const {
        const int nStates = static_cast<int>(st_.size());
        RevGraph rev(nStates);

        for (int from = 0; from < nStates; ++from) {
            if (!reachable[from]) continue;
            const auto &st = st_[from];
            for (const auto &[sym, to]: st.edges) {
                if (to < 0 || to >= nStates) continue;
                if (!reachable[to]) continue;
                rev[to].push_back(RevEdge{sym, from});
            }
        }
        return rev;
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


    template<typename T, typename V>
    void DFA<T, V>::minimalize() {
        int nStates = static_cast<int>(st_.size());

        Partition p{nStates};

        // 1. remove unreachable states
        std::vector reachable(st_.size(), false);
        dfs(start_, reachable);
        auto rev = build_reverse_edges(reachable);

        // 2. initial partition
        auto alphabet = collect_alphabet();

        std::vector<std::vector<RevEdge> > revGraph(nStates);
        for (int from = 0; from < nStates; ++from) {
            if (!reachable[from]) continue;
            for (const auto &st = st_[from];
                 const auto &[sym, to]: st.edges) {
                if (!reachable[to]) continue;
                revGraph[to].push_back(RevEdge{sym, from});
            }
        }

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
                auto X = find_predecessors(A, sym, rev);
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


    template<typename U, typename W=int>
    std::ostream &operator<<(std::ostream &os, const DFA<U, W> &dfa) {
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
                if (std::isprint(static_cast<unsigned char>(sym))) {
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


    // Explicit instantiation for lexer
    // u -- Symbol --> v
    // no additional value on states
    template class DFA<lexer::Symbol>;

    template std::ostream &operator<<<lexer::Symbol>(std::ostream &, const DFA<lexer::Symbol> &);
}
