#include "lexer/regex.h"

#include <iostream>

#include "lexer/dfa.h"


int main() {
    front::lexer::Regex regex{"(ca)+b"};
    try {
        auto nfa = regex.compile(1, 0);

        auto dfa = std::make_unique<front::lexer::DFA>(nfa);
        std::cout << (*dfa) << std::endl;
        dfa->minimalize();
        std::cout << (*dfa) << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
