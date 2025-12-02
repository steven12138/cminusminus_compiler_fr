#include <iostream>
#include <sstream>
#include "lexer/lexer.h"
#include "token.h"
#include "../../include/utils/dfa.h"
#include "utils/timer.h"
using namespace std;


namespace front {
    
    
    inline std::string format_token_for_output(const Token &token) {
        std::ostringstream oss;
        oss << token.lexeme << "\t<";
        
        switch (token.type) {
            // ========== 关键字 KW ==========
            case TokenType::KwInt:
                oss << "KW,1";
                break;
            case TokenType::KwVoid:
                oss << "KW,2";
                break;
            case TokenType::KwReturn:
                oss << "KW,3";
                break;
            case TokenType::KwConst:
                oss << "KW,4";
                break;
            case TokenType::KwMain:
                oss << "KW,5";
                break;
            case TokenType::KwFloat:
                oss << "KW,6";
                break;
            case TokenType::KwIf:
                oss << "KW,7";
                break;
            case TokenType::KwElse:
                oss << "KW,8";
                break;
            
            // ========== 运算符 OP ==========
            case TokenType::OpPlus:
                oss << "OP,6";
                break;
            case TokenType::OpMinus:
                oss << "OP,7";
                break;
            case TokenType::OpMultiply:
                oss << "OP,8";
                break;
            case TokenType::OpDivide:
                oss << "OP,9";
                break;
            case TokenType::OpMod:
                oss << "OP,10";
                break;
            case TokenType::OpAssign:
                oss << "OP,11";
                break;
            case TokenType::OpGreater:
                oss << "OP,12";
                break;
            case TokenType::OpLess:
                oss << "OP,13";
                break;
            case TokenType::OpEqual:
                oss << "OP,14";
                break;
            case TokenType::OpLessEqual:
                oss << "OP,15";
                break;
            case TokenType::OpGreaterEqual:
                oss << "OP,16";
                break;
            case TokenType::OpNotEqual:
                oss << "OP,17";
                break;
            case TokenType::OpAnd:
                oss << "OP,18";
                break;
            case TokenType::OpOr:
                oss << "OP,19";
                break;
            
            // ========== 界符 SE ==========
            case TokenType::SepLParen:
                oss << "SE,20";
                break;
            case TokenType::SepRParen:
                oss << "SE,21";
                break;
            case TokenType::SepLBrace:
                oss << "SE,22";
                break;
            case TokenType::SepRBrace:
                oss << "SE,23";
                break;
            case TokenType::SepSemicolon:
                oss << "SE,24";
                break;
            case TokenType::SepComma:
                oss << "SE,25";
                break;
            
            // ========== 标识符 IDN ==========
            case TokenType::Identifier:
                oss << "IDN," << token.lexeme;
                break;
            
            // ========== 整数 INT ==========
            case TokenType::LiteralInt:
                oss << "INT," << token.lexeme;
                break;
            
            // ========== 浮点数 FLOAT ==========
            case TokenType::LiteralFloat:
                oss << "FLOAT," << token.lexeme;
                break;
            
            // ========== 其他情况 ==========
            case TokenType::EndOfFile:
                // EOF 不输出
                return "";
                break;
            
            default:
                oss << "UNKNOWN";
                break;
        }
        
        oss << ">";
        return oss.str();
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    std::ostringstream buf;
    std::string line;
    INIT_TIMER(Lexer);
    front::lexer::Lexer lexer;
    STOP_TIMER(Lexer);

    while (std::getline(cin, line)) {
        buf << line << '\n';
    }

    const std::string source = buf.str();
    const auto &tokens = lexer.tokenize(source);
    
    // 使用新的格式化函数输出
    for (const auto &t: tokens) {
        std::string formatted = front::format_token_for_output(t);
        if (!formatted.empty()) {  // 跳过 EOF
            cout << formatted << '\n';
        }
    }

    return 0;
}