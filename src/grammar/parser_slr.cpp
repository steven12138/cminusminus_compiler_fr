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

                if (prod.body.size() == 1 && prod.body[0].is_epsilon()) {
                    Item complete_item{std::make_shared<Production>(prod), 1};
                    auto [it2, inserted2] = closure.insert(complete_item);
                    if (inserted2) {
                        q.push(*it2);
                    }
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
                if (item.is_complete()) continue;
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
                        auto existing_it = action_table_.find({k, a});

                        if (existing_it != action_table_.end()) {
                            const auto &existing_action = existing_it->second;

                            // Reduce -> Shift
                            if (existing_action.type == SLRAction::ActionType::Shift) {
                                // Shift First (Resolve in favor of Shift)
                                // resolve dangling-else conflicts in favor of shift
                                continue;
                            }

                            // reduce -> reduce
                            if (existing_action.type == SLRAction::ActionType::Reduce) {
                                std::cerr << "Warning: Reduce/Reduce conflict ignored." << std::endl;
                                continue;
                            }
                        }

                        // no conflict, insert reduce action
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


    static std::string trace_lhs_for_token(const Token &tok) {
        switch (tok.type) {
            case TokenType::Identifier:
                return "Ident";

            case TokenType::LiteralInt:
                return "IntConst";

            case TokenType::LiteralFloat:
                return "floatConst";

            case TokenType::KwInt:
            case TokenType::KwVoid:
            case TokenType::KwReturn:
            case TokenType::KwFloat:
            case TokenType::KwIf:
            case TokenType::KwElse:
            case TokenType::KwConst:
                return tok.lexeme;
            case TokenType::KwMain:
                return "Ident";

            default:
                return tok.lexeme;
        }
    }


    ParseResult SLRParser::parse(const std::vector<Token> &tokens) const {
        const auto &token_map = grammar_.token_to_terminal_;

        std::vector<int> state_stack;
        state_stack.push_back(0); // start state
        std::vector<ast::SemVal> val_stack;

        size_t curr = 0;
        std::vector<ParseStep> result;
        result.reserve(tokens.size() * 2);

        while (!state_stack.empty()) {
            int s = state_stack.back();

            Token current_token;
            Symbol a;

            if (curr < tokens.size()) {
                current_token = tokens[curr];
                if (!token_map.contains(current_token)) {
                    result.emplace_back(Symbol::NonTerminal("ERROR"),
                                        Symbol::Terminal(current_token.lexeme),
                                        Error);

                    std::cerr << "Parse Error! at line: " << current_token.loc.line
                            << ", col: " << current_token.loc.column << std::endl;
                    std::cerr << "unexpected symbol: " << current_token.lexeme << std::endl;


                    return {{}, result, false};
                }
                a = token_map.at(current_token);
            } else {
                std::cerr << "Error: Reached end of input tokens, using End symbol as lookahead." << std::endl;
                return {{}, result, false};
            }


            auto action_it = action_table_.find({s, a});
            if (action_it == action_table_.end()) {
                result.emplace_back(Symbol::NonTerminal("ERROR"), a, Error);
                std::cerr << "Parse Error! at line: " << current_token.loc.line
                        << ", col: " << current_token.loc.column << std::endl;
                std::cerr << "No action for state " << s << " and lookahead " << a.name << std::endl;
                return {{}, result, false};
            }

            switch (const SLRAction &act = action_it->second; act.type) {
                case SLRAction::ActionType::Shift: {
                    // Shift
                    Symbol lhs = T(trace_lhs_for_token(current_token));
                    Symbol rhs = T(current_token.lexeme);
                    result.emplace_back(lhs, rhs, Move);
                    state_stack.push_back(act.target);

                    val_stack.push_back(ast::make_semantic(current_token));

                    if (curr < tokens.size()) {
                        curr++;
                    }
                    break;
                }

                case SLRAction::ActionType::Reduce: {
                    // Reduce
                    const auto &prod = grammar_.productions[act.target];
                    Symbol top_symbol = prod.head;
                    if (prod.trace.has_value()) {
                        result.emplace_back(NT(prod.trace->first), T(prod.trace->second), Reduction);
                    }

                    // pop stack
                    const size_t pop_count = std::ranges::count_if(
                        prod.body, [](const Symbol &sym) { return !sym.is_epsilon(); }
                    );
                    if (state_stack.size() < pop_count) {
                        std::cerr << "Parse Error: State stack underflow during reduce" << std::endl;
                        return {{}, result, false};
                    }

                    std::vector<ast::SemVal> rhs_vals;
                    rhs_vals.reserve(pop_count);
                    {
                        const auto start_it = val_stack.end() - static_cast<int>(pop_count);
                        std::ranges::move(
                            std::ranges::subrange(start_it, val_stack.end()),
                            std::back_inserter(rhs_vals)
                        );
                    }

                    state_stack.resize(state_stack.size() - pop_count);
                    val_stack.resize(val_stack.size() - pop_count);
                    // execute semantic action
                    ast::SemVal new_val{std::monostate{}};
                    if (prod.action) {
                        new_val = prod.action(rhs_vals);
                    }

                    // GOTO
                    if (state_stack.empty()) {
                        std::cerr << "Parse Error: Stack empty after pop" << std::endl;
                        return {nullptr, result, false};
                    }

                    int s_prime = state_stack.back();
                    auto goto_it = goto_table_.find({s_prime, prod.head});

                    if (goto_it == goto_table_.end()) {
                        result.emplace_back(top_symbol, a, Error);
                        std::cerr << "Parse Error: No GOTO entry for state " << s_prime
                                << " and symbol " << prod.head.name << std::endl;
                        return {nullptr, result, false};
                    }

                    state_stack.push_back(goto_it->second);
                    val_stack.push_back(std::move(new_val));
                    break;
                }

                case SLRAction::ActionType::Accept: {
                    result.emplace_back(grammar_.start_symbol_, a == End() ? T("EOF") : a, Accept);
                    ast::ProgramPtr root = nullptr;
                    if (!val_stack.empty()) {
                        if (auto p = std::get_if<ast::ProgramPtr>(&val_stack.back())) {
                            root = std::move(*p);
                        }
                    }
                    return {std::move(root), result, true};
                }
                default: {
                    result.emplace_back(Symbol::NonTerminal("ERROR"), a, Error);
                    std::cerr << "Parse Error: Invalid action type." << std::endl;
                    return {nullptr, result, false};
                }
            }
        }

        return {nullptr, result, false};
    }
}
