#pragma once
#include <string>
#include <vector>

#include "token.h"


namespace front::grammar {
    constexpr std::string EPS{"epsilon"};


    struct Symbol {
        enum class Type {
            Terminal, NonTerminal, Epsilon, End,
        };

        Type type{Type::Terminal};
        std::string name{};


        static Symbol Terminal(const std::string &name) {
            return Symbol{Type::Terminal, name};
        }

        static Symbol NonTerminal(const std::string &name) {
            return Symbol{Type::NonTerminal, name};
        }

        static Symbol Epsilon() {
            return Symbol{Type::Epsilon, EPS};
        }

        static Symbol End() {
            return Symbol{Type::End, "$"};
        }

        bool is_terminal() const { return type == Type::Terminal; }
        bool is_non_terminal() const { return type == Type::NonTerminal; }
        bool is_epsilon() const { return type == Type::Epsilon; }
        bool is_end() const { return type == Type::End; }

        bool operator==(const Symbol &other) const {
            return type == other.type && name == other.name;
        }
    };

    struct SymbolHasher {
        size_t operator()(const Symbol &sym) const noexcept {
            const size_t h1 = std::hash<int>()(static_cast<int>(sym.type));
            const size_t h2 = std::hash<std::string>()(sym.name);
            // hash combine
            return h1 ^ (h2 + 0x9e3779b97f4a7c15ull + (h1 << 6) + (h1 >> 2));
        }
    };

    inline Symbol NT(const std::string &name) {
        return Symbol::NonTerminal(name);
    }

    inline Symbol T(const std::string &name) {
        return Symbol::Terminal(name);
    }

    inline Symbol Epsilon() {
        return Symbol::Epsilon();
    }
}
