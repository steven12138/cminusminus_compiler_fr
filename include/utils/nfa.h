#pragma once

#include <memory>
#include <vector>
#include <limits>

#include "../lexer/symbol.h"


namespace front {
    template<typename T>
    struct NFATrans {
        T sym{};
        int to{-1};
    };

    template<typename T, typename V>
    struct NFAState {
        std::vector<NFATrans<T> > edges;
        int token{-1};
        int priority{std::numeric_limits<int>::max()};
        V val{};
    };


    template<typename T, typename V = int>
    class NFA {
    public:
        static constexpr int EPS = -1;

        NFA() = default;

        int new_state();

        void add_edge(int from, int to, T sym);


        void set_accept(int state, int token, int priority);

        static std::unique_ptr<NFA> union_many(const std::vector<std::unique_ptr<NFA> > &subs);

        int start_state() const { return start_; }

        void set_start(const int state) { start_ = state; }

        std::vector<int> epsilon_closure(const std::vector<int> &state) const;

        std::vector<int> move(const std::vector<int> &states, T target);

        std::vector<T> collect_symbols(const std::vector<int> &set) const;

        std::pair<int, int> computing_accept(const std::vector<int> &state) const;

        int num_states() const { return static_cast<int>(st_.size()); }
        const std::vector<NFAState<T, V> > &states() const { return st_; }
        std::vector<NFAState<T, V> > &states() { return st_; }

        template<typename U, typename W>
        friend std::ostream &operator<<(std::ostream &os, const NFA<U, W> &nfa);

    private:
        std::vector<NFAState<T, V> > st_{};
        int start_{-1};
    };


}
