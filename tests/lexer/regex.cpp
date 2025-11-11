#include "lexer/regex.h"

#include <iostream>


int main() {
    front::lexer::Regex regex{"(ab)*"};
    try {
        auto nfa = regex.compile(1, 0);
        std::cout << (*nfa) << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
