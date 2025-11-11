//
// Created by steven on 11/11/25.
//

#include "../../include/lexer/nfa.h"


int lexer::NFA::new_state() {
    this->st_.push_back({{}, -1,INT_MAX});
    return static_cast<int>(this->st_.size() - 1);
}

void lexer::NFA::add_edge(int from, int to, Symbol sym) {
    this->st_[from].edges.push_back({sym, to});
}

void lexer::NFA::set_accept(int state, int token, int priority) {
    if (auto &st = this->st_[state]; st.priority > priority) {
        st.token = token;
        st.priority = priority;
    }
}

std::unique_ptr<lexer::NFA> lexer::NFA::union_many(const std::vector<std::unique_ptr<NFA> > &subs) {
    std::unique_ptr<NFA> out = std::make_unique<NFA>();

    if (subs.empty()) {
        out->set_start(out->new_state());
        return out;
    }

    int total = 0;
    for (auto &s: subs) total += s->num_states();
    out->st_.reserve(1 + total);

    for (auto &sub: subs) {
        if (sub->num_states() == 0 || sub->start_state() < 0) continue;

        const int base = out->num_states();

        // move states
        out->st_.insert(out->st_.end(),
                        std::make_move_iterator(sub->st_.begin()),
                        std::make_move_iterator(sub->st_.end()));

        // re-map edges
        for (int s = 0; s < sub->num_states(); ++s)
            for (auto &[sym, to]: out->st_[base + s].edges)
                to += base;

        out->add_eps(out->start_state(), base + sub->start_state());
    }
    return out;
}


std::ostream &lexer::operator<<(std::ostream &os, const NFA &nfa) {
    os << "```mermaid\n";
    os << "graph TD;\n";
    os << "  start((start)) --> S" << nfa.start_state() << ";\n";
    for (const auto &state: nfa.states()) {
        if (state.token >= 0) {
            os << "  S" << (&state - &nfa.states()[0]) << "([\"S" << (&state - &nfa.states()[0]) << " (accept rules " <<
                    state.token <<
                    ")\"]);\n";
        } else {
            os << "  S" << (&state - &nfa.states()[0]) << "([\"S" << (&state - &nfa.states()[0]) << "\"]);\n";
        }
        for (const auto &[sym, to]: state.edges) {
            if (sym == EPS) {
                os << "  S" << (&state - &nfa.states()[0]) << " -- ε --> S" << to << ";\n";
            } else if (isprint(sym)) {
                os << "  S" << (&state - &nfa.states()[0]) << " -- '" << static_cast<char>(sym) << "' --> S" << to <<
                        ";\n";
            } else {
                os << "  S" << (&state - &nfa.states()[0]) << " -- [" << sym << "] --> S" << to << ";\n";
            }
        }
    }
    os << "```\n";
    return os;
}
