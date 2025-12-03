#include <iostream>

#include "grammar/grammar.h"
#include "grammar/parser_slr.h"
#include "lexer/lexer.h"
#include "token.h"

using namespace front;

int main() {
    const std::string source = R"(

int fd(int a){return a-1;}

int main(){
    int a=1,b=2,c=3;
    c = fd(c);
    a=b*c;
    return a;
}
)";

    // 1. 词法分析
    lexer::Lexer lexer;
    auto &raw_tokens = lexer.tokenize(source);

    // 关键一步：对 token 进行后处理，把函数定义里的 int/float 识别成 func_int/func_float
    auto tokens = post_process(raw_tokens);

    for (const auto &t: tokens) {
        std::cout << t << std::endl;
    }

    // 2. 构造文法和 SLR 解析器
    grammar::Grammar grammar{};
    grammar::SLRParser parser{std::move(grammar)};

    // 3. 运行 SLR 解析
    auto res = parser.parse(tokens);

    ast::print_ast(res.program, std::cout);

    return 0;
}
