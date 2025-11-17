//
// Created by steven on 11/17/25.
//


#include <iostream>

#include "grammar/grammar.h"

using namespace front::grammar;

int main() {
    /*
     文法G：
        E-> TE'
        E'-> +TE' | ε
        T-> FT'
        T'-> *FT' | ε
        F-> (E) | id
     */
    Grammar g{true};

    std::cout << g << std::endl;
    g.has_back_tracing(std::cout);
    return 0;
}
