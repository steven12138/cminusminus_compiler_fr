#pragma once
#include <algorithm>
#include <memory>
#include <ostream>

#ifdef USE_MAGIC_ENUM
#include <magic_enum/magic_enum.hpp>
#endif


#include "grammar.h"
#include "utils/nfa.h"
#include "utils/util.h"

#include "parser.h"
#include "token.h"
#include <vector>

namespace front::grammar {
    struct Item {
        std::shared_ptr<Production> prod;
        int dot_pos;

        bool operator==(const Item &other) const {
            return prod->id == other.prod->id && dot_pos == other.dot_pos;
        }

        bool operator<(const Item &other) const {
            if (prod->id != other.prod->id) return prod->id < other.prod->id;
            return dot_pos < other.dot_pos;
        }

        Symbol dot() const {
            if (dot_pos >= static_cast<int>(prod->body.size())) {
                return Symbol::Epsilon();
            }
            return prod->body[dot_pos];
        }


        Item next() const {
            return {prod, dot_pos + 1};
        }

        friend std::ostream &operator<<(std::ostream &os, const Item &item) {
            os << item.prod->head.name << " -> ";
            const int n = static_cast<int>(item.prod->body.size());
            for (int i = 0; i < n; i++) {
                if (i == item.dot_pos) os << "· ";
                os << item.prod->body[i].name << " ";
            }
            return os;
        }

        bool is_complete() const {
            return dot_pos >= static_cast<int>(prod->body.size());
        }
    };


    struct SLRAction {
        enum class ActionType {
            Shift, Reduce, Accept, Error
        };

        ActionType type;
        int target = -1; // state id for Shift, production id for Reduce
        SLRAction(ActionType type, int target) : type(type), target(target) {
        }

        static SLRAction shift(int i) {
            return {ActionType::Shift, i};
        }

        static SLRAction reduce(int i) {
            return {ActionType::Reduce, i};
        }

        static SLRAction accept() {
            return {ActionType::Accept, -1};
        }

        static SLRAction error() {
            return {ActionType::Error, -1};
        }

        friend std::ostream &operator<<(std::ostream &os, const SLRAction &obj) {
#ifdef USE_MAGIC_ENUM
            return os << "type: " << magic_enum::enum_name(obj.type)
                   << " target: " << obj.target;
#else
            return os
                   << "type: " << (int) obj.type
                   << " target: " << obj.target;
#endif
        }
    };


    class SLRParser {
    public:
        explicit SLRParser(Grammar grammar);



        void print_item_sets(std::ostream &os) const;

        void print_go_function(std::ostream &os) const;

        void print_action_table(std::ostream &os) const;

        void print_goto_table(std::ostream &os) const;

        std::vector<ParseStep> parse(const std::vector<Token> &tokens) const;
        

    private:
        struct ItemHash {
            size_t operator()(const Item &item) const {
                const size_t pid = std::hash<size_t>()(item.prod->id);
                const size_t dh = std::hash<int>()(item.dot_pos);
                return hash_combine(pid, dh);
            }
        };


        using ItemSetType = std::unordered_set<Item, ItemHash>;

        struct ItemSet {
            int id;
            std::unordered_set<Item, ItemHash> items;
        };


        void closure(std::unordered_set<Item, ItemHash> &closure);

        void init_item_set();

        void calc_action_goto_tables();

        static std::vector<Item> make_key(const ItemSetType &items) {
            std::vector key(items.begin(), items.end());
            std::ranges::sort(key, std::less{});
            return key;
        }

        std::pair<int, bool> add_state(ItemSetType &&items);

        Grammar grammar_;

        NFA<Symbol> nfa_;

        struct ItemKeyHash {
            size_t operator()(const std::vector<Item> &items) const {
                size_t h = 0;
                for (const auto &item: items) {
                    h = hash_combine(h, ItemHash()(item));
                }
                return h;
            }
        };

        std::vector<ItemSet> item_sets_;
        // key requires order
        std::unordered_map<std::vector<Item>, int, ItemKeyHash> state_id_;

        struct GoFuncHash {
            size_t operator()(const std::pair<int, Symbol> &item) const {
                const size_t pid = std::hash<size_t>()(item.first);
                const size_t sh = SymbolHash()(item.second);
                return hash_combine(pid, sh);
            }
        };


        std::unordered_map<std::pair<int, Symbol>, int, GoFuncHash> go_func_;

        std::unordered_map<std::pair<int, Symbol>, SLRAction, GoFuncHash> action_table_;
        std::unordered_map<std::pair<int, Symbol>, int, GoFuncHash> goto_table_;
    };
}
