//
// Created by steven on 12/3/25.
//
#pragma once
#include <memory>

#include "Module.h"
#include "ast/ast.h"

namespace front::ir {
    class IRGenerator {
    public:
        struct Result {
            std::unique_ptr<Module> module;
        };

        static Result generate(const ast::ProgramPtr &program);
    };
}
