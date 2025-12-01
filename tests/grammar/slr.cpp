//
// Created by steven on 12/2/25.
//
#include <iostream>

#include "grammar/grammar.h"
#include "grammar/parser_slr.h"

using namespace front::grammar;

int main() {
    const SLRParser parser{Grammar{}};
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
