#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <optional>

#include "lexer/lexer.h"
#include "grammar/grammar.h"
#include "grammar/parser_slr.h"
#include "ir/ir_generator.h"

using namespace front;

static void print_usage(const char *prog) {
    std::cerr << "Usage:\n"
            << "  " << prog << " [options] <source-file>\n"
            << "\nOptions:\n"
            << "  -o <file>          Write IR output to <file>\n"
            << "  -S                Print IR to stdout (default if no -o)\n"
            << "  --dump-tokens     Print lexer output to stdout\n"
            << "  --dump-parse      Print SLR parse trace to stdout\n"
            << "  --gtrace-only     Parse and print trace only (no IR generation)\n"
            << "  -h, --help        Show help\n"
            << "\nSource file:\n"
            << "  <source-file>     Path to source file (default: stdin)\n"
            << "  -                 Read source from stdin explicitly\n";
}

static std::string read_all_from_stream(std::istream &is) {
    std::ostringstream oss;
    oss << is.rdbuf();
    return oss.str();
}

struct Options {
    std::string input_path;
    std::string output_file;
    bool emit_ir_stdout{true};
    bool dump_tokens{false};
    bool dump_parse{false};
    bool lex_only{false};
    bool gtrace_only{false};
};

static std::optional<Options> parse_args(int argc, char *argv[]) {
    Options opts{};
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            print_usage(argv[0]);
            return std::nullopt;
        }
        if (strcmp(arg, "-o") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: -o requires a filename\n";
                return std::nullopt;
            }
            opts.output_file = argv[++i];
            opts.emit_ir_stdout = false;
            continue;
        }
        if (strcmp(arg, "-S") == 0) {
            opts.emit_ir_stdout = true;
            continue;
        }
        if (strcmp(arg, "--dump-tokens") == 0) {
            opts.dump_tokens = true;
            continue;
        }
        if (strcmp(arg, "--dump-parse") == 0) {
            opts.dump_parse = true;
            continue;
        }
        if (strcmp(arg, "--gtrace-only") == 0) {
            opts.dump_parse = true;
            opts.gtrace_only = true;
            opts.emit_ir_stdout = false;
            opts.output_file.clear();
            continue;
        }
        if (strcmp(arg, "--lex-only") == 0) {
            opts.lex_only = true;
            opts.dump_tokens = true;
            opts.emit_ir_stdout = false;
            continue;
        }
        if (strcmp(arg, "-") == 0) {
            opts.input_path = "-";
            continue;
        }
        if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            return std::nullopt;
        }
        opts.input_path = arg;
    }
    if (opts.input_path.empty()) {
        // default to stdin when no path is provided
        opts.input_path = "-";
    }
    return opts;
}

int main(int argc, char *argv[]) {
    auto opts_opt = parse_args(argc, argv);
    if (!opts_opt) {
        return 1;
    }
    const auto [
        input_path,
        output_file,
        emit_ir_stdout,
        dump_tokens,
        dump_parse,
        lex_only,
        gtrace_only] = *opts_opt;

    try {
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

        lexer::Lexer lexer{std::move(source_code)};
        const auto &tokens = lexer.tokenize();
        if (dump_tokens) {
            lexer::print_tokens(std::cout, tokens);
        }
        if (lex_only) {
            return 0;
        }

        const auto &processed = post_process(tokens);
        grammar::Grammar grammar{};
        const grammar::SLRParser parser{std::move(grammar)};
        const auto &[root, steps, success] = parser.parse(processed);

        if (dump_parse) {
            grammar::print_parse_steps(std::cout, steps);
        }

        if (!success) {
            std::cerr << "Parse error\n";
            return 1;
        }

        if (gtrace_only) {
            return 0;
        }

        auto [module] = ir::IRGenerator::generate(root);
        std::string ir = module->print();

        if (!output_file.empty()) {
            std::ofstream ofs(output_file);
            if (!ofs) {
                std::cerr << "Error: cannot write to output file: " << output_file << "\n";
                return 1;
            }
            ofs << ir;
        }

        if (emit_ir_stdout) {
            std::cout << ir << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
