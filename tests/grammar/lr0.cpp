#include <iostream>

#include "grammar/grammar.h"
#include "grammar/parser_slr.h"

using namespace front::grammar;

int main() {
    /*
     文法G：
    S -> E
    E -> aA | bB
    A -> cA | d
    B -> cB | d
     */
    Grammar g{
        "S",
        {
            {"S", {NT("E")}},
            {"E", {T("a"), NT("A")}},
            {"E", {T("b"), NT("B")}},
            {"A", {T("c"), NT("A")}},
            {"A", {T("d")}},
            {"B", {T("c"), NT("B")}},
            {"B", {T("d")}}
        }
    };
    SLRParser parser{std::move(g)};
    parser.print_item_sets(std::cout);
    std::cout << std::endl;
    parser.print_go_function(std::cout);
    return 0;
}
