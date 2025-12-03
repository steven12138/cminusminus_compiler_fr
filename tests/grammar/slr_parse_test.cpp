#include <iostream>

#include "grammar/grammar.h"
#include "grammar/parser_slr.h"
#include "lexer/lexer.h"
#include "token.h"

using namespace front::grammar;
using namespace front::lexer;

int main() {
    const std::string source = R"( 
int a = 10;

)";

    // 1. 词法分析
    Lexer lexer;
    auto &raw_tokens = lexer.tokenize(source);

    // 关键一步：对 token 进行后处理，把函数定义里的 int/float 识别成 func_int/func_float
    auto tokens = front::post_process(raw_tokens);

    // 2. 构造文法和 SLR 解析器
    Grammar grammar{};
    SLRParser parser{grammar};

    // 3. 运行 SLR 解析
    auto steps = parser.parse(tokens);

    // 4. 输出规约序列（作业要求格式的雏形）
    for (size_t i = 0; i < steps.size(); ++i) {
        const auto &[top, lookahead, action] = steps[i];
        std::cout << (i + 1) << '\t'
                  << top << '#' << lookahead << '\t';
        switch (action) {
            case Move:
                std::cout << "move";
                break;
            case Reduction:
                std::cout << "reduction";
                break;
            case Accept:
                std::cout << "accept";
                break;
            case Error:
                std::cout << "error";
                break;
        }
        std::cout << '\n';
    }

    return 0;
}