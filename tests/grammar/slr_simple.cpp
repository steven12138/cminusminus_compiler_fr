#include <iostream>

#include "grammar/grammar.h"
#include "grammar/parser_slr.h"

using namespace front::grammar;

int main() {
    /*
    文法G：
    (0) S’ -> E
    (1) E -> E+T
    (2) E -> T
    (3) T -> T*F
    (4) T -> F
    (5) F -> (E)
    (6) F -> i
     */
    Grammar g{
        "S'",
        {
            {"S'", {NT("E")}}, // (0)
            {"E", {NT("E"), T("+"), NT("T")}}, // (1)
            {"E", {NT("T")}}, // (2)
            {"T", {NT("T"), T("*"), NT("F")}}, // (3)
            {"T", {NT("F")}}, // (4)
            {"F", {T("("), NT("E"), T(")")}}, // (5)
            {"F", {T("i")}} // (6)",
        },
        false
    };

    std::cout << g << std::endl;
    std::cout << "First set" << std::endl;
    g.print_first_set(std::cout);
    std::cout << "Follow set" << std::endl;
    g.print_follow_set(std::cout);

    SLRParser parser{std::move(g)};
    std::cout << "Item Set" << std::endl;
    parser.print_item_sets(std::cout);
    std::cout << "Go Function" << std::endl;
    parser.print_go_function(std::cout);
    std::cout << "GOTO Table" << std::endl;
    parser.print_goto_table(std::cout);
    std::cout << "Action Table:" << std::endl;
    parser.print_action_table(std::cout);
    return 0;
}
