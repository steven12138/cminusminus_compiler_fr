#pragma once
#include <algorithm>
#include <vector>
#include <limits>

#include "nfa.h"
#include "symbol.h"


namespace front::lexer {
    struct DFATrans {
        Symbol sym;
        int to;
    };

    struct DFAState {
        std::vector<DFATrans> edges;
        int token{-1};
        int priority{std::numeric_limits<int>::max()};
    };

    class DFA {
    public:
        struct Group {
            std::vector<int> states;
            bool is_accepting{false};
            int token{-1};
            int priority{std::numeric_limits<int>::max()};
            bool valid;
        };

        struct Partition {
            std::vector<Group> groups;
            std::vector<int> state_to_group;

            Partition(int nStates) : state_to_group(nStates, -1) {
                groups.reserve(nStates);
            }


            int add_group(std::vector<int> states,
                          bool is_accepting, int token, int priority) {
                const int gid = static_cast<int>(groups.size());
                for (const auto st: states) {
                    state_to_group[st] = gid;
                }
                groups.emplace_back(Group{
                    std::move(states), is_accepting, token, priority, true
                });

                return gid;
            }


            int split(int gid, std::vector<int> &states) {
                auto &old = groups[gid];
                if (!old.valid) return -1;
                std::vector<int> diff, inter;
                std::ranges::sort(old.states);
                std::ranges::sort(states);
                std::ranges::set_difference(old.states, states,
                                            std::back_inserter(diff));
                std::ranges::set_intersection(old.states, states,
                                              std::back_inserter(inter));
                if (inter.empty() || diff.empty()) {
                    return -1;
                }

                old.states = inter;
                for (int state: inter) {
                    state_to_group[state] = gid;
                }
                old.valid = true;

                int new_gid = add_group(std::move(diff), old.is_accepting, old.token, old.priority);
                for (int state: diff) {
                    state_to_group[state] = new_gid;
                }
                return new_gid;
            }

            int find(int state) {
                return state_to_group[state];
            }
        };

        int new_state();

        explicit DFA() = default;

        explicit DFA(const std::unique_ptr<NFA> &);

        void add_edge(int u, int v, Symbol sym);

        int transition(int state, Symbol sym) const;

        friend std::ostream &operator<<(std::ostream &os, const DFA &dfa);

        void dfs(int u, std::vector<bool> &reachable) const;

        std::vector<Symbol> collect_alphabet();

        std::vector<int> find_predecessors(const Group &group, Symbol sym) const;

        void minimalize();

        int start_state() { return start_; }

        const std::vector<DFAState> &states() const { return st_; }

    private:
        std::vector<DFAState> st_;
        int start_{-1};
    };
}
