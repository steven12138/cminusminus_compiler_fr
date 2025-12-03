//
// Simple parser edge-case checks for lexer + SLR parser.
//
#include <cassert>
#include <string>
#include <tuple>
#include <vector>

#include "grammar/grammar.h"
#include "grammar/parser_slr.h"
#include "lexer/lexer.h"
#include "token.h"

using front::post_process;
using front::grammar::Grammar;
using front::grammar::SLRParser;
using front::lexer::Lexer;

static std::tuple<bool, front::ast::ProgramPtr> parse_source(const std::string &src) {
    Lexer lexer{src};
    const auto &tokens = lexer.tokenize();
    const auto processed = post_process(tokens);
    Grammar grammar{};
    const SLRParser parser{std::move(grammar)};
    auto parsed = parser.parse(processed);
    return std::make_tuple(parsed.success, std::move(parsed.program));
}

static void expect_success(const std::string &src) {
    auto [success, root] = parse_source(src);
    assert(success);
    assert(root != nullptr);
}

static void expect_failure(const std::string &src) {
    auto [success, root] = parse_source(src);
    (void) root;
    assert(!success);
}

int main() {
    const std::string basic_positive = R"(
        int main() {
            int a = 1, b = 2;
            a = a + b - 1;
            if (a == 2) {
                return 0;
            } else {
                b = b * 2;
                return b;
            }
        }
    )";
    expect_success(basic_positive);

    const std::string dangling_else = R"(
        int main() {
            int a = 1;
            int b = 2;
            if (a)
                if (b) return 3;
                else return 4;
            return 0;
        }
    )";
    expect_success(dangling_else);

    const std::string missing_semicolon = R"(
        int main() {
            int a = 1
            return a;
        }
    )";
    expect_failure(missing_semicolon);

    const std::string unbalanced_brace = R"(
        int main() {
            if (1) {
                return 1;
        }
    )";
    expect_failure(unbalanced_brace);

    return 0;
}
