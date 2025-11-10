//
// Created by steven on 11/11/25.
//

#include "../../include/nfa.h"


int lexer::NFA::new_state() {
    this->st_.push_back({{}, -1,INT_MAX});
    return static_cast<int>(this->st_.size() - 1);
}

void lexer::NFA::add_edge(int from, int to, Symbol sym) {
    this->st_[from].edges.push_back({sym, to});
}

void lexer::NFA::mark_accept(int state, int token, int priority) {
    if (auto &st = this->st_[state]; st.priority > priority) {
        st.token = token;
        st.priority = priority;
    }
}

lexer::NFA lexer::NFA::union_many(std::vector<NFA> subs) {
    NFA out;

    if (subs.empty()) {
        out.set_start(out.new_state());
        return out;
    }

    size_t total = 0;
    for (auto &s: subs) total += static_cast<size_t>(s.num_states());
    out.st_.reserve(1 + total);

    const int super_start = out.new_state();
    out.set_start(super_start);

    for (auto &sub: subs) {
        if (sub.num_states() == 0 || sub.start_state() < 0) continue;

        const int base = out.num_states();

        // move states
        out.st_.insert(out.st_.end(),
                       std::make_move_iterator(sub.st_.begin()),
                       std::make_move_iterator(sub.st_.end()));

        // re-map edges
        for (int s = 0; s < sub.num_states(); ++s)
            for (auto &tr: out.st_[base + s].edges)
                tr.to += base;

        out.add_eps(super_start, base + sub.start_state());
    }

    return out;
}
