//
// Created by steven on 11/18/25.
//

#pragma once
#include "symbol.h"
#include "ast/ast.h"

namespace front::grammar {
    enum ParseAction {
        Reduction, Move, Accept, Error
    };


    struct ParseStep {
        Symbol top;
        Symbol lookahead;
        ParseAction action{Error};
    };


    struct ParseResult {
        ast::ProgramPtr program;
        std::vector<ParseStep> actions;
        bool success = false;
    };

    inline std::ostream &print_parse_steps(
        std::ostream &os,
        const std::vector<ParseStep> &steps) {
        for (size_t i = 0; i < steps.size(); ++i) {
            const auto &[top, lookahead, action] = steps[i];

            os << (i + 1) << '\t' << top << '#' << lookahead << '\t';
            switch (action) {
                case Move:
                    os << "move";
                    break;
                case Reduction:
                    os << "reduction";
                    break;
                case Accept:
                    os << "accept";
                    break;
                case Error:
                    os << "error";
                    break;
            }
            os << '\n';
        }

        return os;
    }
}
