#pragma once
#include <string>
#include <vector>

#include "token.h"
#include "utils/util.h"


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
            return Symbol{Type::Terminal, "$"};
        }

        bool is_terminal() const { return type == Type::Terminal; }
        bool is_non_terminal() const { return type == Type::NonTerminal; }
        bool is_epsilon() const { return type == Type::Epsilon; }
        bool is_end() const { return name == "$"; }

        bool operator==(const Symbol &other) const {
            return type == other.type && name == other.name;
        }

        friend std::ostream &operator<<(std::ostream &os, const Symbol &sym) {
            os << sym.name;
            return os;
        }
    };

    struct SymbolHash {
        size_t operator()(const Symbol &sym) const noexcept {
            const size_t h1 = std::hash<int>()(static_cast<int>(sym.type));
            const size_t h2 = std::hash<std::string>()(sym.name);
            return hash_combine(h1, h2);
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

    inline Symbol End() {
        return Symbol::End();
    }
}
