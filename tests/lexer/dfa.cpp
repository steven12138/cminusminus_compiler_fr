#include <iostream>
#include <sstream>
#include "lexer/lexer.h"
#include "../../include/utils/dfa.h"
#include "utils/timer.h"
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    std::ostringstream buf;
    std::string line;
    INIT_TIMER(Lexer);
    front::lexer::Lexer lexer;
    STOP_TIMER(Lexer);


    while (std::getline(cin, line)) {
        buf << line << '\n';
    }

    const std::string source = buf.str();
    const auto &token = lexer.tokenize(source);
    for (const auto &t: token) {
        cout << t << '\n';
    }

    return 0;
}
