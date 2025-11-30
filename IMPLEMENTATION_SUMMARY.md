# 编译原理大作业实现总结

## 已完成的功能

### 1. 词法分析器 ✅
- **位置**: `include/lexer/`, `src/lexer/`
- **功能**: 
  - 基于自动机理论实现
  - 正则表达式 → NFA → DFA → 最小化 DFA
  - 识别关键字、标识符、运算符、界符、整数、浮点数
  - 输出 Token 序列

### 2. 语法分析器 (SLR) ✅
- **位置**: `include/grammar/parser_slr.h`, `src/grammar/parser_slr.cpp`
- **功能**:
  - 构造 LR(0) 项目集规范族
  - 计算 FIRST 集和 FOLLOW 集
  - 构造 SLR 预测分析表
  - 实现 SLR 解析算法
  - 在解析过程中构建 AST
  - 输出解析步骤和错误信息

### 3. 抽象语法树 (AST) ✅
- **位置**: `include/ast/ast.h`
- **节点类型**:
  - 表达式节点: IntLiteral, FloatLiteral, Identifier, BinaryExpr, UnaryExpr, CallExpr
  - 语句节点: AssignStmt, ExprStmt, ReturnStmt, IfStmt, BlockStmt
  - 声明节点: VarDecl, ConstDecl, FuncParam, FuncDef
  - 顶层节点: CompUnit

### 4. 语义分析器 ✅
- **位置**: `include/semantic/symbol_table.h`, `src/semantic/symbol_table.cpp`
- **功能**:
  - 符号表管理（支持作用域嵌套）
  - 类型检查（int/float 兼容性）
  - 重复定义检测
  - 未定义标识符检测
  - 类型不匹配检测
  - 常量赋值检测

### 5. 中间代码生成 (IR) ✅
- **位置**: `include/ir/ir_generator.h`, `src/ir/ir_generator.cpp`
- **功能**:
  - 遍历 AST 生成 LLVM IR
  - 支持变量声明和初始化
  - 支持函数定义
  - 支持表达式计算
  - 支持控制流（if-else）
  - 支持返回语句

### 6. 主程序 ✅
- **位置**: `src/main.cpp`
- **功能**:
  - 读取 `.sy` 源文件
  - 调用词法分析器
  - 调用语法分析器（SLR）
  - 执行语义分析
  - 生成 IR 代码
  - 输出到 `.ll` 文件

## 项目结构

```
include/
├── ast/              # AST 节点定义
│   └── ast.h
├── lexer/            # 词法分析器
├── grammar/          # 语法分析器
├── semantic/         # 语义分析器
│   └── symbol_table.h
├── ir/               # IR 生成器
│   └── ir_generator.h
└── token.h

src/
├── ast/              # AST 实现（如果需要）
├── lexer/            # 词法分析器实现
├── grammar/          # 语法分析器实现
│   ├── parser_slr.cpp
│   └── ast_builder.cpp
├── semantic/         # 语义分析器实现
│   └── symbol_table.cpp
├── ir/               # IR 生成器实现
│   └── ir_generator.cpp
└── main.cpp          # 主程序
```

## 使用方法

1. **编译项目**:
```bash
mkdir build
cd build
cmake ..
make
```

2. **运行编译器**:
```bash
./compiler_frontend input.sy
```

3. **输出文件**:
- 控制台输出: 词法分析、语法分析、语义分析结果
- 文件输出: `input.ll` (LLVM IR 代码)

## 注意事项

1. **compiler_ir 子模块**: 
   - 需要初始化 git submodule: `git submodule update --init --recursive`
   - 如果 compiler_ir 接口不同，需要调整 `ir_generator.cpp` 中的实现

2. **AST 构建**:
   - `ast_builder.cpp` 中的 AST 构建逻辑可能需要根据实际文法调整
   - 某些复杂的产生式可能需要额外的处理

3. **IR 生成**:
   - 当前的 IR 生成是简化版本
   - 如果使用提供的 `compiler_ir` 库，需要调用其 API 而不是直接生成字符串

## 待完善的功能

1. **AST 构建**:
   - 某些产生式的 AST 构建逻辑可能需要进一步完善
   - 函数参数列表的处理
   - 复杂表达式的处理

2. **IR 生成**:
   - 如果使用 `compiler_ir` 库，需要集成其 API
   - 临时变量管理
   - 基本块管理

3. **错误处理**:
   - 更详细的错误信息
   - 错误恢复机制

## 测试建议

1. 创建简单的测试文件 `test.sy`:
```c
int main() {
    int a = 10;
    return a;
}
```

2. 运行编译器并检查输出

3. 逐步增加测试用例的复杂度

