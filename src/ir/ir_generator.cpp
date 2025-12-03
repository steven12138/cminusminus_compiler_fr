#include "ir/ir_generator.h"
#include "ir/codegen_context.h"

#include "Module.h"

namespace front::ir {
    IRGenerator::Result IRGenerator::generate(const ast::ProgramPtr &program) {
        auto module = std::make_unique<Module>("cminusminus");
        CodegenContext ctx(std::move(module));
        if (program) {
            program->codegen(ctx);
        }
        return {std::move(ctx.module_ptr)};
    }
}
