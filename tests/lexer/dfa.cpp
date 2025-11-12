#include <iostream>
#include <sstream>
#include "lexer/lexer.h"
#include "lexer/dfa.h"

using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    std::ostringstream buf;
    std::string line;

    while (std::getline(cin, line)) {
        buf << line << '\n';
    }

    std::string source = buf.str();

    front::lexer::Lexer lexer(source);
    const auto &token = lexer.tokenize();
    for (const auto &t : token) {
        cout << t << '\n';
    }

    return 0;
}
