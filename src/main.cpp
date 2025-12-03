#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>

#include "lexer/lexer.h"
#include "grammar/grammar.h"
#include "grammar/parser_slr.h"
#include "ir/ir_generator.h"

using namespace front;

static void print_usage(const char *prog) {
    std::cerr << "Usage:\n"
            << "  " << prog << " [options] <source-file>\n"
            << "\nOptions:\n"
            << "  -o <file>     Write IR output to <file>\n"
            << "  -S            Print IR to stdout (default)\n"
            << "  -h, --help    Show help\n"
            << "\nSource file:\n"
            << "  <source-file>   Path to source file\n"
            << "  -               Read source from stdin\n";
}

static std::string read_all_from_stream(std::istream &is) {
    std::ostringstream oss;
    oss << is.rdbuf();
    return oss.str();
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string output_file;
    bool print_ir = true; // default behavior
    std::string input_path;

    // ---- Parse command line ----
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(arg, "-o") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: -o requires a filename\n";
                return 1;
            }
            output_file = argv[++i];
            print_ir = false; // writing to file, not stdout
        } else if (strcmp(arg, "-S") == 0) {
            print_ir = true;
        } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        } else {
            input_path = arg; // source file path
        }
    }

    if (input_path.empty()) {
        std::cerr << "Error: no source file provided\n\n";
        print_usage(argv[0]);
        return 1;
    }

    // ---- Read source code ----
    std::string source_code;
    if (input_path == "-") {
        source_code = read_all_from_stream(std::cin);
    } else {
        std::ifstream ifs(input_path);
        if (!ifs) {
            std::cerr << "Error: cannot open input file: " << input_path << "\n";
            return 1;
        }
        source_code = read_all_from_stream(ifs);
    }

    // try {
    // ---- Run compiler front-end ----
    lexer::Lexer lexer{std::move(source_code)};
    const auto &tokens = lexer.tokenize();

    lexer::print_tokens(std::cout, tokens);

    const auto &processed = post_process(tokens);

    grammar::Grammar grammar{};
    const grammar::SLRParser parser{std::move(grammar)};
    const auto &[root, steps, success] = parser.parse(processed);

    grammar::print_parse_steps(std::cout, steps);

    if (!success) {
        std::cerr << "Parse error\n";
        return 1;
    }

    auto ir_module = ir::IRGenerator::generate(root);
    std::string ir = ir_module.module->print();

    if (!output_file.empty()) {
        std::ofstream ofs(output_file);
        if (!ofs) {
            std::cerr << "Error: cannot write to output file: " << output_file << "\n";
            return 1;
        }
        ofs << ir;
    }

    if (print_ir) {
        std::cout << ir << std::endl;
    }
    // } catch (const std::exception &e) {
    //     std::cerr << "Exception: " << e.what() << std::endl;
    //     return 1;
    // }

    return 0;
}
