#include "lexer/regex.h"


namespace front::lexer {
    std::unique_ptr<::lexer::NFA> Regex::compile(int token, int priority) {
        auto parser = RegexParser(pattern, false);

        if (pattern[0] == '?' && pattern[1] == 'i' && pattern[2] == ':') {
            parser.insensitive = true;
            parser.pos = 3;
        }

        auto frag = parser.parse_regex();
        if (frag.is_invalid() || !parser.at_end()) {
            throw std::runtime_error("Invalid regex pattern: " + pattern);
        }

        parser.nfa->set_start(frag.start);
        parser.nfa->set_accept(frag.accept, token, priority);
        return std::move(parser.nfa);
    }

    Regex::NFAFrag Regex::RegexParser::parse_regex() {
        return parse_alt();
    }

    Regex::NFAFrag Regex::RegexParser::parse_alt() {
        std::vector<NFAFrag> branches;
        branches.push_back(parse_concat());
        while (curr() == '|') {
            consume();
            NFAFrag next = parse_concat();
            if (branches.back().is_invalid() || next.is_invalid()) {
                return NFAFrag::invalid();
            }
            branches.push_back(next);
        }

        if (branches.size() == 1) {
            return branches.front();
        }


        const auto out = empty_fragment();
        for (const auto &frag: branches) {
            nfa->add_edge(out.start, frag.start, ::lexer::EPS);
            nfa->add_edge(frag.accept, out.accept, ::lexer::EPS);
        }
        return out;
    }

    Regex::NFAFrag Regex::RegexParser::parse_concat() {
        std::vector<NFAFrag> frags;

        while (true) {
            auto frag = parse_repeat();
            if (frag.is_invalid()) break;
            frags.push_back(frag);
        }

        if (frags.empty()) {
            return NFAFrag::invalid();
        }

        for (size_t i = 1; i < frags.size(); ++i) {
            nfa->add_edge(frags[i - 1].accept, frags[i].start, ::lexer::EPS);
        }

        return {frags.front().start, frags.back().accept};
    }

    Regex::NFAFrag Regex::RegexParser::parse_repeat() {
        auto f = parse_atom();
        if (f.is_invalid()) return f;

        while (true) {
            if (char c = curr(); c == '*' || c == '+') {
                consume();
                auto res = empty_fragment();
                nfa->add_edge(res.start, f.start, ::lexer::EPS);
                nfa->add_edge(f.accept, res.accept, ::lexer::EPS);
                nfa->add_edge(f.accept, f.start, ::lexer::EPS);

                if (c == '*')
                    nfa->add_edge(res.start, res.accept, ::lexer::EPS);

                f = res;
            } else break;
        }
        return f;
    }

    Regex::NFAFrag Regex::RegexParser::parse_atom() {
        char c = curr();
        if (c == '\0' || c == '|' || c == ')') {
            return NFAFrag::invalid();
        }

        if (c == '(') {
            consume(); // consume '('
            auto f = parse_alt();
            if (curr() != ')') {
                return NFAFrag::invalid();
            }
            consume(); // consume ')'
            return f;
        }

        if (c == '.') {
            consume();
            auto f = empty_fragment();
            nfa->add_edge(f.start, f.accept, ::lexer::ANY);
            return f;
        }

        if (c == '\\') {
            consume();
            c = consume();
            auto f = empty_fragment();
            if (insensitive && isalpha(c)) {
                nfa->add_edge(f.start, f.accept, tolower(c));
                nfa->add_edge(f.start, f.accept, toupper(c));
            } else {
                nfa->add_edge(f.start, f.accept, c);
            }
            return f;
        }

        consume();
        auto f = empty_fragment();
        if (insensitive && isalpha(c)) {
            nfa->add_edge(f.start, f.accept, tolower(c));
            nfa->add_edge(f.start, f.accept, toupper(c));
        } else {
            nfa->add_edge(f.start, f.accept, c);
        }
        return f;
    }
}
