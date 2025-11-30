#include "grammar/parser_slr.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <stack>

#include "grammar/grammar.h"
#include "ast/ast.h"
#include "token.h"


namespace front::grammar {
    SLRParser::SLRParser(Grammar grammar) : grammar_(std::move(grammar)) {
        init_item_set();
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


    void SLRParser::closure(std::unordered_set<Item, ItemHash> &I) {
        std::queue<Item> q;
        for (const auto &it: I) q.push(it);

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
                auto [iter, inserted] = I.insert(new_item);
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

            // for X,  GO(I, X) = closure(J_X)
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
    
    std::unique_ptr<ast::CompUnitNode> SLRParser::parse(const std::vector<Token>& tokens) {
        parse_steps_.clear();
        
        // 构建 ACTION 和 GOTO 表
        std::unordered_map<std::pair<int, Symbol>, std::string, GoFuncHash> action_table;
        std::unordered_map<std::pair<int, Symbol>, int, GoFuncHash> goto_table;
        
        // 构建 ACTION 表（移进和归约）
        for (size_t i = 0; i < item_sets_.size(); ++i) {
            const auto& item_set = item_sets_[i];
            for (const auto& item : item_set.items) {
                Symbol X = item.dot();
                
                if (X != Symbol::Epsilon()) {
                    // 移进项
                    if (X.is_terminal() && go_func_.contains({static_cast<int>(i), X})) {
                        int next_state = go_func_.at({static_cast<int>(i), X});
                        action_table[{static_cast<int>(i), X}] = "s" + std::to_string(next_state);
                    }
                } else {
                    // 归约项 [A -> α.]
                    const auto& prod = *item.prod;
                    if (prod.head.name == grammar_.start_symbol_.name) {
                        // 接受
                        action_table[{static_cast<int>(i), Symbol::End()}] = "acc";
                    } else {
                        // 归约
                        const auto& follow = grammar_.follow_set_.at(prod.head);
                        for (const auto& a : follow) {
                            if (a.is_terminal() || a.is_end()) {
                                action_table[{static_cast<int>(i), a}] = "r" + std::to_string(prod.id);
                            }
                        }
                    }
                }
            }
        }
        
        // 构建 GOTO 表
        for (const auto& [key, next_state] : go_func_) {
            const auto& [state, sym] = key;
            if (sym.is_non_terminal()) {
                goto_table[{state, sym}] = next_state;
            }
        }
        
        // 开始解析
        std::stack<int> state_stack;
        std::stack<std::unique_ptr<ast::ASTNode>> value_stack;
        std::stack<Token> token_stack;
        
        state_stack.push(0);
        size_t token_idx = 0;
        
        while (token_idx < tokens.size()) {
            int state = state_stack.top();
            const Token& token = tokens[token_idx];
            
            // 将 Token 转换为 Symbol
            Symbol a;
            if (token.type == TokenType::EndOfFile) {
                a = Symbol::End();
            } else if (grammar_.token_to_terminal_.contains(token)) {
                a = grammar_.token_to_terminal_.at(token);
            } else {
                std::cerr << "Parse Error: Unknown token at line " << token.loc.line 
                         << ", col " << token.loc.column << std::endl;
                return nullptr;
            }
            
            auto action_key = std::make_pair(state, a);
            
            if (action_table.contains(action_key)) {
                std::string action = action_table.at(action_key);
                
                if (action[0] == 's') {
                    // 移进
                    int next_state = std::stoi(action.substr(1));
                    state_stack.push(next_state);
                    // 对于终结符，创建对应的 AST 节点（如果是标识符或字面量）
                    if (token.type == TokenType::Identifier) {
                        auto id_node = std::make_unique<ast::IdentifierNode>(token.lexeme, token.loc);
                        value_stack.push(std::move(id_node));
                    } else if (token.type == TokenType::LiteralInt) {
                        int value = std::stoi(token.lexeme);
                        auto int_node = std::make_unique<ast::IntLiteralNode>(value, token.loc);
                        value_stack.push(std::move(int_node));
                    } else if (token.type == TokenType::LiteralFloat) {
                        double value = std::stod(token.lexeme);
                        auto float_node = std::make_unique<ast::FloatLiteralNode>(value, token.loc);
                        value_stack.push(std::move(float_node));
                    }
                    token_idx++;
                    
                    parse_steps_.push_back({Symbol::Terminal(""), a, Move});
                } else if (action[0] == 'r') {
                    // 归约
                    size_t prod_id = std::stoul(action.substr(1));
                    const auto& prod = grammar_.productions[prod_id];
                    
                    // 弹出 |β| 个状态和值
                    std::vector<std::unique_ptr<ast::ASTNode>> children;
                    size_t pop_count = 0;
                    for (const auto& sym : prod.body) {
                        if (!sym.is_epsilon()) {
                            pop_count++;
                        }
                    }
                    
                    for (size_t i = 0; i < pop_count; ++i) {
                        state_stack.pop();
                        if (!value_stack.empty()) {
                            children.insert(children.begin(), std::move(value_stack.top()));
                            value_stack.pop();
                        }
                    }
                    
                    // 构建 AST 节点
                    size_t token_start = (token_idx > pop_count) ? token_idx - pop_count : 0;
                    auto ast_node = buildASTNode(prod, children, tokens, token_start, token_idx);
                    
                    // GOTO
                    int new_state = state_stack.top();
                    auto goto_key = std::make_pair(new_state, prod.head);
                    if (goto_table.contains(goto_key)) {
                        int next_state = goto_table.at(goto_key);
                        state_stack.push(next_state);
                        if (ast_node) {
                            value_stack.push(std::move(ast_node));
                        }
                    } else {
                        std::cerr << "Parse Error: GOTO table error" << std::endl;
                        return nullptr;
                    }
                    
                    parse_steps_.push_back({prod.head, a, Reduction});
                } else if (action == "acc") {
                    // 接受
                    parse_steps_.push_back({grammar_.start_symbol_, a, Accept});
                    if (!value_stack.empty()) {
                        return std::unique_ptr<ast::CompUnitNode>(
                            dynamic_cast<ast::CompUnitNode*>(value_stack.top().release()));
                    }
                    return std::make_unique<ast::CompUnitNode>(Location{1, 1});
                }
            } else {
                std::cerr << "Parse Error: No action at state " << state 
                         << " for symbol " << a.name 
                         << " at line " << token.loc.line 
                         << ", col " << token.loc.column << std::endl;
                return nullptr;
            }
        }
        
        std::cerr << "Parse Error: Unexpected end of input" << std::endl;
        return nullptr;
    }
}
