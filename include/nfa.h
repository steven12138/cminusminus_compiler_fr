#pragma once

#include <climits>
#include <vector>

#include "symbol.h"


namespace lexer {
    struct NFATrans {
        Symbol sym{};
        int to{-1};
    };

    struct NFAState {
        std::vector<NFATrans> edges;
        int token{-1};
        int priority{INT_MAX};
    };

    class NFA {
    public:
        NFA() = default;

        int new_state();

        void add_edge(int from, int to, Symbol sym);

        void add_eps(int from, int to) {
            add_edge(from, to, EPS);
        }

        void mark_accept(int state, int token, int priority);

        static NFA union_many(std::vector<NFA> subs);

        int start_state() const { return start_; }

        void set_start(const int state) { start_ = state; }

        int num_states() const { return static_cast<int>(st_.size()); }
        const std::vector<NFAState> &states() const { return st_; }
        std::vector<NFAState> &states() { return st_; }

    private:
        std::vector<NFAState> st_{};
        int start_{-1};
    };
}
