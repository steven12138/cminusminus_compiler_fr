#include <iostream>

#include "lexer/lexer.h"
#include "lexer/dfa.h"


using namespace std;

int main() {
    front::lexer::Lexer lexer("int main() { return 42; }");
    const auto &token = lexer.tokenize();
    for (const auto &t: token) {
        cout << t << endl;
    }
    return 0;
}
