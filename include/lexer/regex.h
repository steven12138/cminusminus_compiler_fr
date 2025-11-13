#pragma once
#include <memory>
#include <string>
#include <utility>

#include "../utils/nfa.h"


namespace front::lexer {
    class Regex {
    public:
        std::string pattern{};

        explicit Regex(std::string pattern) : pattern(std::move(pattern)) {
        }

        std::unique_ptr<NFA<Symbol> > compile(int token, int priority) const;

    private:
        struct NFAFrag {
            int start;
            int accept;

            NFAFrag(int start, int end) : start(start), accept(end) {
            }

            static NFAFrag invalid() {
                return {-1, 1};
            }

            bool is_invalid() const {
                return start < 0 || accept < 0;
            }
        };

        struct RegexParser {
            std::string pattern{};
            size_t pos{};
            bool insensitive{};
            std::unique_ptr<NFA<Symbol> > nfa;

            explicit RegexParser(std::string pattern, bool insensitive = false)
                : pattern(std::move(pattern)), pos(0), insensitive(insensitive),
                  nfa(std::make_unique<NFA<Symbol> >()) {
            }

            char curr() const {
                return this->pattern[pos];
            }

            char consume() {
                return this->pattern[pos++];
            }

            NFAFrag empty_fragment() const {
                return {nfa->new_state(), nfa->new_state()};
            }


            /**
             * Grammar:
             * regex   := alt
             * alt     := concat ('|' concat)*
             * concat  := repeat+
             * repeat  := atom '*'
             *         := atom '+'
             * atom    := '(' alt ')'
             *         := '.'
             *         := '\' escape
             *         := literal
             */

            NFAFrag parse_regex();

            NFAFrag parse_alt();

            NFAFrag parse_concat();

            NFAFrag parse_repeat();

            NFAFrag parse_atom();

            bool at_end() const {
                return curr() == '\0';
            }
        };
    };
}
