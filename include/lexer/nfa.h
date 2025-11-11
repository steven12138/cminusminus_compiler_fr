#pragma once

#include <memory>
#include <vector>
#include <limits>

#include "symbol.h"


namespace lexer {
    struct NFATrans {
        Symbol sym{};
        int to{-1};
    };

    struct NFAState {
        std::vector<NFATrans> edges;
        int token{-1};
        int priority{std::numeric_limits<int>::max()};
    };

    class NFA {
    public:
        NFA() = default;

        int new_state();

        void add_edge(int from, int to, Symbol sym);

        void add_eps(const int from, const int to) {
            add_edge(from, to, EPS);
        }

        void set_accept(int state, int token, int priority);

        static std::unique_ptr<NFA> union_many(const std::vector<std::unique_ptr<NFA> > &subs);

        int start_state() const { return start_; }

        void set_start(const int state) { start_ = state; }

        int num_states() const { return static_cast<int>(st_.size()); }
        const std::vector<NFAState> &states() const { return st_; }
        std::vector<NFAState> &states() { return st_; }

        friend std::ostream &operator<<(std::ostream &os, const NFA &nfa);

    private:
        std::vector<NFAState> st_{};
        int start_{-1};
    };
}
