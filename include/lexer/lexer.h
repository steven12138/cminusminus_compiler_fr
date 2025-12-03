#pragma once
#include <memory>
#include <string>
#include <vector>

#include "../utils/nfa.h"
#include "token.h"
#include "utils/dfa.h"

#define RULE_CAPS "A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z"
#define RULE_LOWERS "a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z"
#define RULE_DIGITS "0|1|2|3|4|5|6|7|8|9"
#define RULE_ID_START RULE_CAPS "|" RULE_LOWERS "|_"
#define RULE_ID_CHAR RULE_CAPS "|" RULE_LOWERS "|" RULE_DIGITS "|_"
#define RULE_FLOAT "((" RULE_DIGITS ")+\\.(" RULE_DIGITS ")*|(" RULE_DIGITS ")*\\.(" RULE_DIGITS ")+)"

namespace front::lexer {
    class Lexer {
    public:
        explicit Lexer(std::string source);

        explicit Lexer();

        std::vector<Token> &tokenize();

        std::vector<Token> &tokenize(const std::string &source);


        void optimize();

        std::string source_;

        std::unique_ptr<DFA<Symbol> > dfa;
        std::vector<Token> tokens;

    private:
        int row{0}, column{0};
        std::vector<std::tuple<std::string, TokenType, TokenCategory> > rules;

        std::unique_ptr<NFA<Symbol> > init_rules();

        void advance(const std::string &lexeme);
    };

    std::ostream &print_tokens(std::ostream &os, const std::vector<Token> &token);
}
