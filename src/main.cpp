#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "lexer/lexer.h"
#include "grammar/parser_slr.h"
#include "semantic/symbol_table.h"
#include "ir/ir_generator.h"

using namespace front::lexer;
using namespace front::grammar;
using namespace front::semantic;
using namespace front::ir;
using namespace front::ast;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file.sy>" << std::endl;
        return 1;
    }
    
    std::string input_file = argv[1];
    
    // 读取源文件
    std::ifstream file(input_file);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << input_file << std::endl;
        return 1;
    }
    
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();
    
    std::cout << "=== 词法分析 ===" << std::endl;
    
    // 词法分析
    Lexer lexer(source);
    auto& tokens = lexer.tokenize();
    
    std::cout << "识别到 " << tokens.size() << " 个 Token:" << std::endl;
    for (size_t i = 0; i < tokens.size() && i < 20; ++i) {
        std::cout << "  " << tokens[i] << std::endl;
    }
    if (tokens.size() > 20) {
        std::cout << "  ... (还有 " << (tokens.size() - 20) << " 个 Token)" << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "=== 语法分析 (SLR) ===" << std::endl;
    
    // 语法分析
    Grammar grammar(false);  // false 表示使用 SLR
    SLRParser parser(grammar);
    
    auto ast_root = parser.parse(tokens);
    
    if (!ast_root) {
        std::cerr << "语法分析失败！" << std::endl;
        return 1;
    }
    
    std::cout << "语法分析成功！" << std::endl;
    std::cout << "解析步骤数: " << parser.getParseSteps().size() << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== 语义分析 ===" << std::endl;
    
    // 语义分析
    SemanticAnalyzer semantic_analyzer;
    bool semantic_ok = semantic_analyzer.analyze(ast_root.get());
    
    if (!semantic_ok) {
        std::cerr << "语义分析失败！" << std::endl;
        const auto& errors = semantic_analyzer.getErrors();
        for (const auto& error : errors) {
            std::cerr << "  " << error << std::endl;
        }
        return 1;
    }
    
    std::cout << "语义分析成功！" << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== 中间代码生成 ===" << std::endl;
    
    // IR 生成
    IRGenerator ir_generator;
    std::string ir_code = ir_generator.generate(ast_root.get());
    
    std::cout << "生成的 LLVM IR 代码:" << std::endl;
    std::cout << ir_code << std::endl;
    
    // 输出到文件
    std::string output_file = input_file;
    size_t pos = output_file.find_last_of('.');
    if (pos != std::string::npos) {
        output_file = output_file.substr(0, pos);
    }
    output_file += ".ll";
    
    std::ofstream out_file(output_file);
    if (out_file.is_open()) {
        out_file << ir_code;
        out_file.close();
        std::cout << "IR 代码已保存到: " << output_file << std::endl;
    } else {
        std::cerr << "警告: 无法写入文件 " << output_file << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "=== 编译完成 ===" << std::endl;
    
    return 0;
}
