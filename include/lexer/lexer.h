#pragma once
#include <string>
#include <vector>

#include "token.h"


namespace front::lexer {
    class Lexer {
    public:
        explicit Lexer(std::string source);

        const std::vector<Token> &tokenize();
    };
}
