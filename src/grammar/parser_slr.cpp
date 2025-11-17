#include "grammar/parser_slr.h"

#include <algorithm>
#include <iostream>
#include <queue>

#include "grammar/grammar.h"


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
}
