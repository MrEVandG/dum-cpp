#pragma once

#include <string>
#include <vector>
#include <optional>
#include <iostream>

enum class TokenType {
    exit,         // 0
    int_lit,      // 1
    semicolon,    // 2
    open_paren,   // 3
    close_paren,  // 4
    ident,        // 5
    let,          // 6
    equals,       // 7
    plus,         // 8
    star,         // 9
    dash,         // 10
    fslash,       // 11
    open_brace,   // 12
    close_brace,  // 13
    if_,          // 14
    function,     // 15
    comma,        // 16
    greater_than, // 17
    less_than,    // 18
};

const char* TokenTypes[] = {
    "exit",         // 0
    "int_lit",      // 1
    "semicolon",    // 2
    "open_paren",   // 3
    "close_paren",  // 4
    "ident",        // 5
    "let",          // 6
    "equals",       // 7
    "plus",         // 8
    "star",         // 9
    "dash",         // 10
    "fslash",       // 11
    "open_brace",   // 12
    "close_brace",  // 13
    "if_",          // 14
    "function",     // 15
    "comma",        // 16
    "greater_than", // 17
    "less_than",    // 18
};

std::optional<int> bin_prec(TokenType type)
{
    switch (type) {
    case TokenType::dash:
    case TokenType::plus:
        return 0;
    case TokenType::fslash:
        return 1;
    case TokenType::star:
        return 2;
    case TokenType::greater_than: // 14 + 4 > 8 + 7 -> 18 > 15 -> 1
    case TokenType::less_than:
        return 3;
    default:
        return {};
    }
}

struct Token {
    TokenType type;
    std::optional<std::string> value {};
};

class Tokenizer {
public:
    inline explicit Tokenizer(std::string src)
        : m_src(std::move(src))
    {
    }

    inline std::vector<Token> tokenize()
    {
        std::vector<Token> tokens;
        std::string buf;
        while (peek().has_value()) {
            if (std::isalpha(peek().value())) {
                buf.push_back(consume());
                while (peek().has_value() && std::isalnum(peek().value())) {
                    buf.push_back(consume());
                }
                if (buf == "exit") {
                    tokens.push_back({ .type = TokenType::exit });
                    buf.clear();
                }
                else if (buf == "function") {
                    tokens.push_back({ .type = TokenType::function });
                    buf.clear();
                }
                else if (buf == "let") {
                    tokens.push_back({ .type = TokenType::let });
                    buf.clear();
                }
                else if (buf == "if") {
                    tokens.push_back({ .type = TokenType::if_ });
                    buf.clear();
                }
                else {
                    tokens.push_back({ .type = TokenType::ident, .value = buf });
                    buf.clear();
                }
            }
            else if (std::isdigit(peek().value())) {
                buf.push_back(consume());
                while (peek().has_value() && std::isdigit(peek().value())) {
                    buf.push_back(consume());
                }
                tokens.push_back({ .type = TokenType::int_lit, .value = buf });
                buf.clear();
            }
            else if (peek().value() == '(') {
                consume();
                tokens.push_back({ .type = TokenType::open_paren });
            }
            else if (peek().value() == ')') {
                consume();
                tokens.push_back({ .type = TokenType::close_paren });
            }
            else if (peek().value() == ';') {
                consume();
                tokens.push_back({ .type = TokenType::semicolon });
            }
            else if (peek().value() == ',') {
                consume();
                tokens.push_back({ .type = TokenType::comma });
            }
            else if (peek().value() == '=') {
                consume();
                tokens.push_back({ .type = TokenType::equals });
            }
            else if (peek().value() == '+') {
                consume();
                tokens.push_back({ .type = TokenType::plus });
            }
            else if (peek().value() == '*') {
                consume();
                tokens.push_back({ .type = TokenType::star });
            }
            else if (peek().value() == '-') {
                consume();
                tokens.push_back({ .type = TokenType::dash });
            }
            else if (peek().value() == '/') {
                consume();
                tokens.push_back({ .type = TokenType::fslash });
            }
            else if (peek().value() == '{') {
                consume();
                tokens.push_back({ .type = TokenType::open_brace });
            }
            else if (peek().value() == '}') {
                consume();
                tokens.push_back({ .type = TokenType::close_brace });
            }
            else if (std::isspace(peek().value())) {
                consume();
            }
            else {
                std::cerr << "`" << peek().value() << "` is not a proper token!! Add the token or just get better!" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        m_index = 0;
        return tokens;
    }

private:
    [[nodiscard]] inline std::optional<char> peek(int offset = 0) const
    {
        if (m_index + offset >= m_src.length()) {
            return {};
        }
        else {
            return m_src.at(m_index + offset);
        }
    }

    inline char consume()
    {
        return m_src.at(m_index++);
    }

    const std::string m_src;
    size_t m_index = 0;
};