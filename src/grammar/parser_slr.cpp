#include "grammar/parser_slr.h"

#include <algorithm>
#include <iostream>
#include <queue>


#include "grammar/grammar.h"
#include "ast/ast.h"

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
}
