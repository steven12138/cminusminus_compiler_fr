## c-- compiler frontend

> TJU compiler final project - c-- compiler frontend

### Structure

some part of this project is still under development, the structure may change in the future.

`compiler_ir` is provided by TA, imported as git submodule.

```
external/
    compiler_ir/ # llvm ir provided by TA
include/
    ast/          # abstract syntax tree
    lexer/        # lexical analyzer
    parser/       # syntax analyzer
    semantic/     # semantic analyzer
src/
    main.cpp      # entry point
    ast/          # abstract syntax tree implementation
    lexer/        # lexical analyzer implementation
    parser/       # syntax analyzer implementation
tests/
    lexer/
        regex.cpp # test regex -> nfa
    unit-tests/   # unit tests
```