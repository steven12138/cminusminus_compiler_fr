#pragma once
#include <algorithm>
#include <vector>
#include <limits>
#include <iterator>

#include "nfa.h"


namespace front {
    template<typename T>
    struct DFATrans {
        T sym;
        int to;
    };

    template<typename T, typename V>
    struct DFAState {
        std::vector<DFATrans<T> > edges;
        int token{-1};
        int priority{std::numeric_limits<int>::max()};
        V val{};
    };


    template<typename T, typename V=int>
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

        explicit DFA(const std::unique_ptr<NFA<T> > &);

        void add_edge(int u, int v, T sym);

        int transition(int state, T sym) const;


        void dfs(int u, std::vector<bool> &reachable) const;

        std::vector<T> collect_alphabet();

        std::vector<int> find_predecessors(const Group &group, T sym,
                                           const std::vector<std::vector<DFATrans<T> > > &rev) const;

        using RevEdge = DFATrans<T>;
        using RevGraph = std::vector<std::vector<RevEdge> >;

        RevGraph build_reverse_edges(const std::vector<bool> &reachable) const;

        void minimalize();

        int start_state() const { return start_; }

        const std::vector<DFAState<T, V> > &states() const { return st_; }


        template<typename U, typename W>
        friend std::ostream &operator<<(std::ostream &os, const DFA<U, W> &dfa);

    private:
        std::vector<DFAState<T, V> > st_;
        int start_{-1};
    };
}
