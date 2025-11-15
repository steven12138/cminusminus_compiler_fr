#pragma once
#include <algorithm>
#include <memory>
#include <ostream>

#include "grammar.h"
#include "utils/nfa.h"
#include "utils/util.h"


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
                if (i == item.dot_pos) os << "* ";
                os << item.prod->body[i].name << " ";
            }
            return os;
        }
    };

    struct ItemHasher {
        size_t operator()(const Item &item) const {
            const size_t pid = std::hash<size_t>()(item.prod->id);
            const size_t dh = std::hash<int>()(item.dot_pos);
            return hash_combine(pid, dh);
        }
    };

    class SLRParser {
    public:
        explicit SLRParser(Grammar grammar);


        void print_item_sets(std::ostream &os) const;

        void print_go_function(std::ostream &os) const;

    private:
        using ItemSetType = std::unordered_set<Item, ItemHasher>;

        struct ItemSet {
            int id;
            std::unordered_set<Item, ItemHasher> items;
        };


        void closure(std::unordered_set<Item, ItemHasher> &closure);

        void init_item_set();

        static std::vector<Item> make_key(const ItemSetType &items) {
            std::vector key(items.begin(), items.end());
            std::ranges::sort(key, std::less{});
            return key;
        }

        std::pair<int, bool> add_state(ItemSetType &&items);

        Grammar grammar_;

        NFA<Symbol> nfa_;

        struct ItemKeyHasher {
            size_t operator()(const std::vector<Item> &items) const {
                size_t h = 0;
                for (const auto &item: items) {
                    h = hash_combine(h, ItemHasher()(item));
                }
                return h;
            }
        };

        std::vector<ItemSet> item_sets_;
        // key requires order
        std::unordered_map<std::vector<Item>, int, ItemKeyHasher> state_id_;

        struct GoFuncHasher {
            size_t operator()(const std::pair<int, Symbol> &item) const {
                const size_t pid = std::hash<size_t>()(item.first);
                const size_t sh = SymbolHasher()(item.second);
                return hash_combine(pid, sh);
            }
        };

        std::unordered_map<std::pair<int, Symbol>, int, GoFuncHasher> go_func_;
    };
}
