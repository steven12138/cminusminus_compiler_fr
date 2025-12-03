#include <iostream>
#include <sstream>

#include "grammar/grammar.h"

using namespace front::grammar;

int main() {
    /*
     文法G：
         S’ -> E
         E -> E+T
         E -> T
         T -> T*F
         T -> F
         F -> (E)
         F -> i
     */
    Grammar g{
        "S'",
        {
            {"S'", {NT("E")}},
            {"E", {NT("E"), T("+"), NT("T")}},
            {"E", {NT("T")}},
            {"T", {NT("T"), T("*"), NT("F")}},
            {"T", {NT("F")}},
            {"F", {T("("), NT("E"), T(")")}},
            {"F", {T("i")}}
        }
    };
    std::stringstream ss{};
    g.print_follow_set(ss);
    return 0;
}
