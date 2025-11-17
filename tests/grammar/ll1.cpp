//
// Created by steven on 11/17/25.
//


#include <iostream>

#include "grammar/grammar.h"

using namespace front::grammar;

int main() {
    /*
     文法G：
        R→Sa | a
        Q→Rb | b
        S→Qc |c
     */
    Grammar g{
        "S",
        {
            {"R", {NT("S"), T("a")}},
            {"R", {T("a")}},
            {"Q", {NT("R"), T("b")}},
            {"Q", {T("b")}},
            {"S", {NT("Q"), T("c")}},
            {"S", {T("c")}}
        },
        true
    };

    std::cout << g << std::endl;
    return 0;
}
