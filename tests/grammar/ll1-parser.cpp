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
VoID MaIn ( ) {            @
  CoNsT InT _X1 = 42 , y = 0 ;    #
  FLOAT Pi = 3.14 , rate = 0.5 ;  $
  int A = 1 , B = 2 ;             ^

  y = y + _X1 * 2 - 5 / 3 % 2 ;   |
  A = A + B ;                     &

  if ( ( y >= 10 && y != 0 ) || ( A < B ) ) {
      y = y + 1 ;
  } else {
      y = y - 1 ;
  }

  if ( Pi > 1.0 && rate <= 1.0 ) {
      A = ( A + B ) * ( _X1 - 3 ) ;
  } else {
      if ( A == B ) { B = B + 1 ; } else { B = B - 1 ; }
  }

  return ;
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
