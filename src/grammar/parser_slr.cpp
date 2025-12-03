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
    std::vector<ParseStep> SLRParser::parse(const std::vector<Token> &tokens) const {
    const auto &token_map = grammar_.token_to_terminal_;
    
    // 1. 状态栈（SLR 用状态栈）
    std::vector<int> state_stack;
    state_stack.push_back(0);  // 初始状态 I0
    
    // 2. 输入指针
    size_t ip = 0;
    
    // 3. 结果序列
    std::vector<ParseStep> result;
    result.reserve(tokens.size() * 2);
    
    // 4. 主循环
    while (true) {
        // 检查输入是否结束
        if (ip >= tokens.size()) {
            int s = state_stack.back();
            result.emplace_back(
                Symbol::NonTerminal("ERROR"),
                Symbol::End(),
                Error
            );
            std::cerr << "Parse Error: Unexpected end of input at state " << s << std::endl;
            break;
        }
        
        // 5. 获取当前状态和输入符号
        int s = state_stack.back();
        const Token &tok = tokens[ip];
        
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
        Symbol a = token_map.at(tok);
        
        // 6. 查 ACTION 表
        auto action_it = action_table_.find({s, a});
        
        if (action_it == action_table_.end()) {
            // ACTION 表没有项 => 语法错误
            result.emplace_back(
                Symbol::NonTerminal("ERROR"),
                a,
                Error
            );
            std::cerr << "Parse Error! at line: " << tok.loc.line 
                      << ", col: " << tok.loc.column << std::endl;
            std::cerr << "No action for state " << s << " and symbol " << a.name << std::endl;
            break;
        }
        
        const SLRAction &act = action_it->second;
        Symbol top_symbol;  // 栈顶符号（根据动作类型确定）
        
        // 7. 执行动作
        if (act.type == SLRAction::ActionType::Shift) {
            // Shift（移进）：栈顶符号 = 当前输入符号（终结符）
            top_symbol = a;
            
            result.emplace_back(top_symbol, a, Move);
            state_stack.push_back(act.target);
            ++ip;
            
        } else if (act.type == SLRAction::ActionType::Reduce) {
            // Reduce（归约）：栈顶符号 = 产生式头部（非终结符）
            int prod_id = act.target;
            const auto &prod = grammar_.productions[prod_id];
            top_symbol = prod.head;
            
            result.emplace_back(top_symbol, a, Reduction);
            
            // 根据产生式体长度弹栈（只弹非ε符号）
            int pop_count = 0;
            for (const auto &sym : prod.body) {
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
            
        } else if (act.type == SLRAction::ActionType::Accept) {
            // Accept（接受）：栈顶符号 = 开始符号
            top_symbol = grammar_.start_symbol_;
            
            result.emplace_back(top_symbol, a, Accept);
            break;
            
        } else {
            // Error
            top_symbol = Symbol::NonTerminal("ERROR");
            result.emplace_back(top_symbol, a, Error);
            std::cerr << "Parse Error! at line: " << tok.loc.line 
                      << ", col: " << tok.loc.column << std::endl;
            break;
        }
    }
    
    return result;
}
}

