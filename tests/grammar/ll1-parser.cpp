//
// Created by steven on 11/17/25.
//
#include <iostream>

#include "grammar/grammar.h"
#include "grammar/parser_ll.h"
#include "grammar/parser_slr.h"
#include "lexer/lexer.h"

using namespace front::grammar;
using namespace front::lexer;

int main() {
    std::string source = R"(
int a = 10;
int main() {
    a = 10;
    return 0;
}
    )";

    Lexer lexer{};
    auto &raw_tokens = lexer.tokenize(source);


    LL1Parser parser{};
    const auto &tokens = parser.preprocess_tokens(raw_tokens);

    const auto &result = parser.parse(tokens);
    for (size_t i = 0; i < result.size(); i++) {
        const auto &[top, lookahead, action] = result[i];
        std::cout << i << "\t" << top << "#" << lookahead << "\t";
        switch (action) {
            case Move:
                std::cout << "move";
                break;
            case Reduction:
                std::cout << "reduce";
                break;
            case Accept:
                std::cout << "accept";
                break;
            case Error:
                std::cout << "error";
                break;
        }
        std::cout << std::endl;
    }
    return 0;
}
