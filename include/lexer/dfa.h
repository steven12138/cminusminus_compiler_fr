#pragma once
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
        int new_state();

        explicit DFA(const std::unique_ptr<NFA> &);

        void add_edge(int u, int v, Symbol sym);

        int transition(int state, Symbol sym) const;

        friend std::ostream &operator<<(std::ostream &os, const DFA &dfa);

        void minimalize();

        int start_state() { return start_; }

        const std::vector<DFAState> &states() const { return st_; }

    private:
        std::vector<DFAState> st_;
        int start_{-1};
    };
}
