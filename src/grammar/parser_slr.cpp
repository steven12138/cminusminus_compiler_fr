#include "grammar/parser_slr.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include<vector>
#include <string>


#include "grammar/grammar.h"
#include "grammar/parser.h"
#include "ast/ast.h"
#include "token.h"

namespace front::grammar {
    SLRParser::SLRParser(Grammar grammar) : grammar_(std::move(grammar)) {
        init_item_set();

        calc_action_goto_tables();
    }

    void SLRParser::print_item_sets(std::ostream &os) const {
        for (const auto &item_set: item_sets_) {
            os << "I" << item_set.id << ":\n";
            for (const auto &item: item_set.items) {
                os << item << std::endl;
            }
            os << std::endl;
        }
    }

    void SLRParser::print_go_function(std::ostream &os) const {
        for (const auto &[key, to_state]: go_func_) {
            const auto &[from_state, sym] = key;
            os << "GO(I" << from_state << ", " << sym.name << ") = I" << to_state << std::endl;
        }
    }


    void SLRParser::print_action_table(std::ostream &os) const {
        for (const auto &[key, action]: action_table_) {
            const auto &[state, sym] = key;
            os << "ACTION[" << state << ", " << sym.name << "] = " << action << std::endl;
        }
    }

    void SLRParser::print_goto_table(std::ostream &os) const {
        for (const auto &[key, to_state]: goto_table_) {
            const auto &[from_state, sym] = key;
            os << "GOTO[" << from_state << ", " << sym.name << "] = " << to_state << std::endl;
        }
    }


    void SLRParser::closure(std::unordered_set<Item, ItemHash> &closure) {
        std::queue<Item> q;
        for (const auto &it: closure) q.push(it);

        while (!q.empty()) {
            auto item = q.front();
            q.pop();

            const auto &sym = item.dot();
            if (!sym.is_non_terminal()) continue;

            auto it = grammar_.production_map.find(sym.name);
            if (it == grammar_.production_map.end()) continue;

            for (auto prod_id: it->second) {
                const auto &prod = grammar_.productions[prod_id];
                Item new_item{std::make_shared<Production>(prod), 0};
                auto [iter, inserted] = closure.insert(new_item);
                if (inserted) {
                    q.push(*iter);
                }
            }
        }
    }


    void SLRParser::init_item_set() {
        // I0 = closure({ [S' -> .S] })
        ItemSetType start_items;
        const auto &start_prod = grammar_.productions[0];
        start_items.insert(Item{std::make_shared<Production>(start_prod), 0});
        closure(start_items);

        auto [start_id, is_new] = add_state(std::move(start_items));

        std::queue<int> workList;
        workList.push(start_id);

        while (!workList.empty()) {
            int I_id = workList.front();
            workList.pop();
            const auto &curr_set = item_sets_[I_id].items;

            std::unordered_map<Symbol, ItemSetType, SymbolHash> symbol_groups;

            // for each item [A -> α.Xβ] in I, group by X
            // insert A -> αX.β , collect in symbol_groups
            for (const auto &item: curr_set) {
                Symbol X = item.dot();
                if (X == Symbol::Epsilon()) continue;

                auto &J = symbol_groups[X];
                J.insert(item.next());
            }

            // for X in symbols:  GO(I, X) = closure(J_X)
            for (auto &[X, kernel]: symbol_groups) {
                closure(kernel);
                auto [J_id, inserted] = add_state(std::move(kernel));
                go_func_[{I_id, X}] = J_id;
                if (inserted) {
                    workList.push(J_id);
                }
            }
        }
    }

    void SLRParser::calc_action_goto_tables() {
        // step1. goto table
        // for each GO(I, A) = J where A is non-terminal, GOTO[I, A] = J
        goto_table_.clear();
        std::ranges::copy_if(
            go_func_, std::inserter(goto_table_, goto_table_.end()),
            [](const auto &pair) {
                return pair.first.second.is_non_terminal();
            }
        );

        // step2. action table
        // for item  A -> alpha . a beta in I_k, GO(I_k, a) = I_j and a is terminal
        // ACTION[k, a] = shift j, "sj"
        //
        // for item A -> alpha . in I_k, for all terminal a, a in FOLLOW(A)
        // ACTION[k, a] = reduce A -> alpha, "rj", A-> alpha is G''s j-th production
        //
        // if item S' -> S . in I_k
        // ACTION[k, $] = accept, "acc"
        int n_states = static_cast<int>(item_sets_.size());
        for (int k = 0; k < n_states; ++k) {
            const auto &I_k = item_sets_[k];

            // A -> alpha . a beta
            for (const auto &item: I_k.items) {
                if (item.is_complete()) continue;
                Symbol a = item.dot();
                if (!a.is_terminal()) continue;
                auto it = go_func_.find({k, a});
                if (it != go_func_.end()) {
                    int j = it->second;
                    action_table_.insert_or_assign(
                        {k, a},
                        SLRAction::shift(j)
                    );
                }
            }

            // A -> alpha .
            for (const auto &item: I_k.items) {
                if (!item.is_complete()) continue;
                const auto &prod = *item.prod;
                if (prod.head == grammar_.start_symbol_) {
                    if (prod.id != 0) {
                        throw std::runtime_error("Invalid start production id");
                    }
                    // S' -> S .
                    action_table_.insert_or_assign(
                        {k, Symbol::End()},
                        SLRAction::accept()
                    );
                } else {
                    // A -> alpha .
                    for (const auto &follow_set = grammar_.follow_set_[prod.head];
                         const auto &a: follow_set) {
                        action_table_.insert_or_assign(
                            {k, a},
                            SLRAction::reduce(static_cast<int>(prod.id))
                        );
                    }
                }
            }
        }
    }

    std::pair<int, bool> SLRParser::add_state(ItemSetType &&items) {
        auto key = make_key(items);
        if (const auto it = state_id_.find(key);
            it != state_id_.end()) {
            return {it->second, false};
        }

        int id = static_cast<int>(item_sets_.size());
        ItemSet set{id, std::move(items)};
        item_sets_.push_back(std::move(set));
        state_id_.emplace(std::move(key), id);
        return {id, true};
    }

    ParseResult SLRParser::parse(const std::vector<Token> &tokens) const {
        const auto &token_map = grammar_.token_to_terminal_;

        std::vector<int> state_stack;
        state_stack.push_back(0);

        size_t curr = 0;

        // 3. 结果序列
        std::vector<ParseStep> result;
        result.reserve(tokens.size() * 2);

        // 4. 主循环
        while (true) {
            int s = state_stack.back();
            Symbol a;
            Token debug_tok; // 用于记录当前 token 信息 (用于错误报告)
            bool is_eof = false;

            // 5. 获取当前状态和输入符号
            if (curr < tokens.size()) {
                // 存在实际输入 Token
                const Token &tok = tokens[curr];
                debug_tok = tok; // 记录 Token 信息用于后续的错误打印

                // Token -> Symbol 转换
                if (!token_map.contains(tok)) {
                    result.emplace_back(
                        Symbol::NonTerminal("ERROR"),
                        Symbol::Terminal(tok.lexeme),
                        Error
                    );
                    std::cerr << "Parse Error! at line: " << tok.loc.line
                            << ", col: " << tok.loc.column << std::endl;
                    std::cerr << "Token not in grammar: " << tok.lexeme << std::endl;
                    break;
                }
                a = token_map.at(tok);
            } else if (curr == tokens.size()) {
                // **核心修复点：输入流耗尽，模拟 Lookahead 为 EOF ($)**
                a = Symbol::End();
                is_eof = true;
            } else {
                // ip > tokens.size()，内部逻辑错误，不应该发生
                std::cerr << "Internal Parse Error: Input pointer advanced past end of stream." << std::endl;
                break;
            }

            // 6. 查 ACTION 表
            auto action_it = action_table_.find({s, a});

            if (action_it == action_table_.end()) {
                // ACTION 表没有项 => 语法错误
                result.emplace_back(
                    Symbol::NonTerminal("ERROR"),
                    a,
                    Error
                );

                if (is_eof) {
                    std::cerr << "Parse Error: No action for state " << s << " and End of File ($)" << std::endl;
                } else {
                    std::cerr << "Parse Error! at line: " << debug_tok.loc.line
                            << ", col: " << debug_tok.loc.column << std::endl;
                    std::cerr << "No action for state " << s << " and symbol " << a.name << std::endl;
                }
                break;
            }

            const SLRAction &act = action_it->second;
            Symbol top_symbol; // 栈顶符号（根据动作类型确定）

            // 7. 执行动作
            switch (act.type) {
                case SLRAction::ActionType::Shift: {
                    // Shift（移进）：栈顶符号 = 当前输入符号（终结符）
                    top_symbol = a;

                    result.emplace_back(top_symbol, a, Move);
                    state_stack.push_back(act.target);
                    // 只有在 Shift 实际 Token 时才移动指针
                    if (!is_eof) {
                        ++curr;
                    }
                    break;
                }
                case SLRAction::ActionType::Reduce: {
                    // Reduce（归约）：栈顶符号 = 产生式头部（非终结符）
                    int prod_id = act.target;
                    const auto &prod = grammar_.productions[prod_id];
                    top_symbol = prod.head;

                    result.emplace_back(top_symbol, a, Reduction);

                    // 根据产生式体长度弹栈（只弹非ε符号）
                    int pop_count = 0;
                    for (const auto &sym: prod.body) {
                        if (!sym.is_epsilon()) {
                            ++pop_count;
                        }
                    }

                    // 弹栈
                    for (int i = 0; i < pop_count && !state_stack.empty(); ++i) {
                        state_stack.pop_back();
                    }

                    if (state_stack.empty()) {
                        result.emplace_back(top_symbol, a, Error);
                        std::cerr << "Parse Error: State stack empty during reduce" << std::endl;
                        break;
                    }

                    // 查 GOTO 表
                    int s2 = state_stack.back();
                    auto goto_it = goto_table_.find({s2, prod.head});
                    if (goto_it == goto_table_.end()) {
                        result.emplace_back(top_symbol, a, Error);
                        std::cerr << "Parse Error: No GOTO entry for state " << s2
                                << " and non-terminal " << prod.head.name << std::endl;
                        break;
                    }
                    int s3 = goto_it->second;
                    state_stack.push_back(s3);
                    break;
                }
                case SLRAction::ActionType::Accept: {
                    // Accept（接受）：栈顶符号 = 开始符号
                    top_symbol = grammar_.start_symbol_;

                    result.emplace_back(top_symbol, a, Accept);
                    // 成功接受，返回成功结果
                    return {nullptr, result, true};
                }
                default: {
                    // Error (理论上不应发生，因为 action_it 已经找到)
                    top_symbol = Symbol::NonTerminal("ERROR");
                    result.emplace_back(top_symbol, a, Error);
                    std::cerr << "Internal Parse Error: Unhandled action type." << std::endl;
                    break; // 统一通过 break 退出 while 循环
                }
            }
        }

        // 如果是通过 break 退出，则认为是失败（尽管 ParseResult 返回 true 表示尝试已结束）
        return {
            {}, result, false // 退出循环即为解析失败，返回 false
        };
    }
}
