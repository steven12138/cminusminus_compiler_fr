#pragma once
#include <algorithm>
#include <memory>
#include <ostream>
#include <stack>
#include <vector>

#include "grammar.h"
#include "parser.h"
#include "utils/nfa.h"
#include "utils/util.h"
#include "../token.h"


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

    struct ItemHash {
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
        
        // 解析函数，返回 AST 根节点
        std::unique_ptr<class front::ast::CompUnitNode> parse(const std::vector<class front::Token>& tokens);
        
        // 获取解析步骤（用于调试）
        std::vector<ParseStep> getParseSteps() const { return parse_steps_; }

    private:
        using ItemSetType = std::unordered_set<Item, ItemHash>;

        struct ItemSet {
            int id;
            std::unordered_set<Item, ItemHash> items;
        };


        void closure(std::unordered_set<Item, ItemHash> &closure);

        void init_item_set();

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
        
        // 解析相关
        std::vector<ParseStep> parse_steps_;
        
        // 构建 AST 的辅助函数
        std::unique_ptr<class front::ast::ASTNode> buildASTNode(
            const Production& prod, 
            const std::vector<std::unique_ptr<class front::ast::ASTNode>>& children,
            const std::vector<class front::Token>& tokens,
            size_t token_start, size_t token_end);
    };
}
