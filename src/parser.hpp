#pragma once

#include <cassert>
#include <variant>

#include "./arena.hpp"
#include "tokenization.hpp"

struct NodeTermIntLit {
    Token int_lit;
};

struct NodeTermIdent {
    Token ident;
};

struct NodeExpr;

struct NodeTermParen {
    NodeExpr* expr;
};

struct NodeBinExprAdd {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprMulti {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprSub {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprDiv {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExpr {
    std::variant<NodeBinExprAdd*, NodeBinExprMulti*, NodeBinExprSub*, NodeBinExprDiv*> var;
};

struct NodeTerm {
    std::variant<NodeTermIntLit*, NodeTermIdent*, NodeTermParen*> var;
};

struct NodeExpr {
    std::variant<NodeTerm*, NodeBinExpr*> var;
};

struct NodeStmtExit {
    NodeExpr* expr;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmt;

struct NodeScope {
    std::vector<NodeStmt*> stmts;
};

struct NodeStmtFunction {
    Token ident; // name of function
    NodeScope* scope;
    std::vector<Token> parameters;
};

struct NodeStmtIf {
    NodeExpr* expr;
    NodeScope* scope;
};

struct NodeStmt {
    std::variant<NodeStmtExit*, NodeStmtLet*, NodeScope*, NodeStmtIf*, NodeStmtFunction*> var;
};

struct NodeProg {
    std::vector<NodeStmt*> stmts;
};

class Parser {
public:
    inline explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens))
        , m_allocator(1024 * 1024 * 4) // 4 mb
    {
    }

    std::optional<NodeTerm*> parse_term()
    {
        if (auto int_lit = try_consume(TokenType::int_lit)) {
            auto term_int_lit = m_allocator.alloc<NodeTermIntLit>();
            term_int_lit->int_lit = int_lit.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_int_lit;
            return term;
        }
        else if (auto ident = try_consume(TokenType::ident)) {
            auto expr_ident = m_allocator.alloc<NodeTermIdent>();
            expr_ident->ident = ident.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = expr_ident;
            return term;
        }
        else if (auto open_paren = try_consume(TokenType::open_paren)) {
            auto expr = parse_expr();
            if (!expr.has_value()) {
                std::cerr << "Expected expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paren, "Expected `)`");
            auto term_paren = m_allocator.alloc<NodeTermParen>();
            term_paren->expr = expr.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_paren;
            return term;
        }
        else {
            return {};
        }
    }

    std::optional<NodeExpr*> parse_expr(int min_prec = 0)
    {
        std::optional<NodeTerm*> term_lhs = parse_term();
        if (!term_lhs.has_value()) {
            return {};
        }
        auto expr_lhs = m_allocator.alloc<NodeExpr>();
        expr_lhs->var = term_lhs.value();

        while (true) {
            std::optional<Token> curr_tok = peek();
            std::optional<int> prec;
            if (curr_tok.has_value()) {
                prec = bin_prec(curr_tok->type);
                if (!prec.has_value() || prec < min_prec) {
                    break;
                }
            }
            else {
                break;
            }
            Token op = consume();
            int next_min_prec = prec.value() + 1;
            auto expr_rhs = parse_expr(next_min_prec);
            if (!expr_rhs.has_value()) {
                std::cerr << "Unable to parse expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            auto expr = m_allocator.alloc<NodeBinExpr>();
            auto expr_lhs2 = m_allocator.alloc<NodeExpr>();
            if (op.type == TokenType::plus) {
                auto add = m_allocator.alloc<NodeBinExprAdd>();
                expr_lhs2->var = expr_lhs->var;
                add->lhs = expr_lhs2;
                add->rhs = expr_rhs.value();
                expr->var = add;
            }
            else if (op.type == TokenType::star) {
                auto multi = m_allocator.alloc<NodeBinExprMulti>();
                expr_lhs2->var = expr_lhs->var;
                multi->lhs = expr_lhs2;
                multi->rhs = expr_rhs.value();
                expr->var = multi;
            }
            else if (op.type == TokenType::dash) {
                auto sub = m_allocator.alloc<NodeBinExprSub>();
                expr_lhs2->var = expr_lhs->var;
                sub->lhs = expr_lhs2;
                sub->rhs = expr_rhs.value();
                expr->var = sub;
            }
            else if (op.type == TokenType::fslash) {
                auto div = m_allocator.alloc<NodeBinExprDiv>();
                expr_lhs2->var = expr_lhs->var;
                div->lhs = expr_lhs2;
                div->rhs = expr_rhs.value();
                expr->var = div;
            }
            else {
                assert(false); // Unreachable - all binary operators have been checked for
            }
            expr_lhs->var = expr;
        }
        return expr_lhs;
    }

    std::optional<NodeScope*> parse_scope()
    {
        if (!try_consume(TokenType::open_brace).has_value()) {
            return {}; // It's not a scope
        }
        auto scope = m_allocator.alloc<NodeScope>();
        while (auto stmt = parse_stmt()) {
            scope->stmts.push_back(stmt.value());
        }
        try_consume(TokenType::close_brace, "Expected `}` to close scope");
        return scope;
    }

    std::optional<NodeStmt*> parse_stmt()
    {
        if (peek().value().type == TokenType::exit && peek(1).has_value()
            && peek(1).value().type == TokenType::open_paren) {
            consume();
            consume();
            auto stmt_exit = m_allocator.alloc<NodeStmtExit>();
            if (auto node_expr = parse_expr()) {
                stmt_exit->expr = node_expr.value();
            }
            else {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paren, "Expected `)` after exit expression");
            try_consume(TokenType::semicolon, "Expected `;` after exit");
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_exit;
            return stmt;
        }
        else if (
            peek().has_value() && peek().value().type == TokenType::let && peek(1).has_value()
            && peek(1).value().type == TokenType::ident && peek(2).has_value()
            && peek(2).value().type == TokenType::equals) {
            consume();
            auto stmt_let = m_allocator.alloc<NodeStmtLet>();
            stmt_let->ident = consume();
            consume();
            if (auto expr = parse_expr()) {
                stmt_let->expr = expr.value();
            }
            else {
                std::cerr << "Invalid expression for variable value" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::semicolon, "Expected `;` after variable declaration");
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_let;
            return stmt;
        }
        else if (peek().has_value() && peek().value().type == TokenType::open_brace) {
            if (auto scope = parse_scope()) {
                auto stmt = m_allocator.alloc<NodeStmt>();
                stmt->var = scope.value();
                return stmt;
            }
            else {
                std::cerr << "Invalid scope" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else if (auto if_ = try_consume(TokenType::if_)) {
            try_consume(TokenType::open_paren, "Expected `(` following if keyword");
            auto stmt_if = m_allocator.alloc<NodeStmtIf>();
            if (auto expr = parse_expr()) {
                stmt_if->expr = expr.value();
            }
            else {
                std::cerr << "Invalid expression for if statement clause" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paren, "Expected `)` for if statement");
            if (auto scope = parse_scope()) {
                stmt_if->scope = scope.value();
            }
            else {
                std::cerr << "Invalid scope for if statement" << std::endl;
                exit(EXIT_FAILURE);
            }
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_if;
            return stmt;
        } else if (auto function = try_consume(TokenType::function)) {
            Token ident = try_consume(TokenType::ident, "Expected function name following function keyword");
            auto stmt_func = m_allocator.alloc<NodeStmtFunction>();
            stmt_func->ident = ident;
            try_consume(TokenType::open_paren, "Expected open parenthesis for function parameters");
            std::vector<Token> parameters;
            while (peek().has_value() && peek().value().type != TokenType::close_paren) {
                parameters.push_back(consume());
                // expect either a comma (another parameter) or close paren
                if (peek().has_value() && peek().value().type != TokenType::close_paren) {
                    try_consume(TokenType::comma, "Expected comma to seperate parameters in function declaration");
                }
            }
            consume(); // consume the )
            stmt_func->parameters = parameters;
            if (auto scope = parse_scope()) {
                stmt_func->scope = scope.value();
            } else {
                std::cerr << "Invalid scope for function statements" << std::endl;
                exit(EXIT_FAILURE);
            }
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_func;
            return stmt;
        }
        else {
            return {};
        }
    }

    std::optional<NodeProg> parse_prog()
    {
        NodeProg prog;
        while (peek().has_value()) {
            if (auto stmt = parse_stmt()) {
                prog.stmts.push_back(stmt.value());
            }
            else {
                std::cerr << "Invalid statement" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        return prog;
    }

    // -------------------- PRETTY PRINTING - CONVERT AST INTO STRING ----------------------


    std::string bin_expr_to_string(const NodeBinExpr* bin_expr) {
        std::stringstream stream;
        struct Visitor {
            std::stringstream& stream;
            Parser& parser;
            void operator()(const NodeBinExprAdd* add) const {
                stream << "{\"type\":\"add\",\"left\":";
                stream << parser.expr_to_string(add->lhs);
                stream << ",\"right\":";
                stream << parser.expr_to_string(add->rhs);
                stream << "}";
            }
            void operator()(const NodeBinExprSub* sub) const {
                stream << "{\"type\":\"sub\",\"left\":";
                stream << parser.expr_to_string(sub->lhs);
                stream << ",\"right\":";
                stream << parser.expr_to_string(sub->rhs);
                stream << "}";
            }
            void operator()(const NodeBinExprDiv* div) const {
                stream << "{\"type\":\"sub\",\"left\":";
                stream << parser.expr_to_string(div->lhs);
                stream << ",\"right\":";
                stream << parser.expr_to_string(div->rhs);
                stream << "}";
            }
            void operator()(const NodeBinExprMulti* mul) const {
                stream << "{\"type\":\"mul\",\"left\":";
                stream << parser.expr_to_string(mul->lhs);
                stream << ",\"right\":";
                stream << parser.expr_to_string(mul->rhs);
                stream << "}";
            }
        };
        Visitor visitor ({.stream = stream,.parser=*this});
        std::visit(visitor, bin_expr->var);
        return stream.str();
    }

    std::string term_to_string(const NodeTerm* term) {
        std::stringstream stream;
        struct Visitor {
            std::stringstream& stream;
            Parser& parser;
            void operator()(const NodeTermIdent* ident) {
                stream << "{\"type\":\"identifier\",\"identifier\":\"" << ident->ident.value.value() << "}";
            }
            void operator()(const NodeTermIntLit* int_lit) {
                stream << "{\"type\":\"integer_literal\",\"value\":" << int_lit->int_lit.value.value() << "}";
            }
            void operator()(const NodeTermParen* parenthesis) {
                stream << "{\"type\":\"parenthesis\",\"value\":" << parser.expr_to_string(parenthesis->expr) << "}";
            }
        };
        Visitor visitor ({.stream=stream,.parser=*this});
        std::visit(visitor,term->var);
        return stream.str();
    }

    std::string expr_to_string(const NodeExpr* expr) {
        std::stringstream stream;
        struct Visitor {
            std::stringstream& stream;
            Parser& parser;
            void operator()(const NodeTerm* term) const {
                stream << "{\"type\":\"term\",\"variant\":";
                stream << parser.term_to_string(term);
                stream << "}";
            }
            void operator()(const NodeBinExpr* bin_expr) const {
                stream << "{\"type\":\"binary_expression\",\"operation\":";
                stream << parser.bin_expr_to_string(bin_expr);
                stream << "}";
            }
        };
        Visitor visitor ({.stream = stream, .parser = *this});
        std::visit(visitor,expr->var);
        return stream.str();
    }

    std::string stmt_to_string(const NodeStmt* stmt) {
        std::stringstream stream;
        struct Visitor {
            std::stringstream& stream;
            Parser& parser;
            void operator()(const NodeStmtExit* exit_stmt) const {
                stream << "{\"type\":\"exit_statement\",\"expression\":";
                stream << parser.expr_to_string(exit_stmt->expr);
                stream << "}";
            }
            void operator()(const NodeStmtFunction* func_stmt) const {
                stream << "{\"type\":\"function_statement\",\"identifier\":\"" << func_stmt->ident.value.value() << "\",\"parameters\":[";
                for (int i = 0; i < func_stmt->parameters.size(); i++) {
                    stream << "\"" << func_stmt->parameters.at(i).value.value() << "\",";
                }
                stream << "],\"body\":[";
                for (int i = 0; i < func_stmt->scope->stmts.size(); i++) {
                    stream << parser.stmt_to_string(func_stmt->scope->stmts.at(i)) << ",";
                }
                stream << "]}";
            }
            void operator()(const NodeStmtIf* if_stmt) const {
                stream << "{\"type\":\"if_statement\",\"expression\":";
                stream << parser.expr_to_string(if_stmt->expr);
                stream << ",\"body\":[";
                for (int i = 0; i < if_stmt->scope->stmts.size(); i++) {
                    stream << parser.stmt_to_string(if_stmt->scope->stmts.at(i)) << ",";
                }
                stream << "]}";
            }
            void operator()(const NodeStmtLet* let_stmt) const {
                stream << "{\"type\":\"var_declaration_statement\",\"identifier\":\"" << let_stmt->ident.value.value() << "\",\"expression\":";
                stream << parser.expr_to_string(let_stmt->expr);
                stream << "}";
            }
            void operator()(const NodeScope* scope) const {
                stream << "{\"type\":\"scope\",\"body\":[";
                for (int i = 0; i < scope->stmts.size(); i++) {
                    stream << parser.stmt_to_string(scope->stmts.at(i));
                }
                stream << "]}";
            }
        };
        Visitor visitor({.stream=stream,.parser=*this});
        std::visit(visitor,stmt->var);
        return stream.str();
    }

    std::string prog_to_string(const NodeProg program) {
        std::stringstream stream;
        stream << "{\"type\":\"program\",\"statements\":[";
        for (int i = 0; i < program.stmts.size(); i++) {
            stream << stmt_to_string(program.stmts.at(i));
        }
        stream << "]}";
        return stream.str();
    }

private:
    [[nodiscard]] inline std::optional<Token> peek(int offset = 0) const
    {
        if (m_index + offset >= m_tokens.size()) {
            return {};
        }
        else {
            return m_tokens.at(m_index + offset);
        }
    }

    inline Token consume()
    {
        return m_tokens.at(m_index++);
    }

    inline Token try_consume(TokenType type, const std::string& err_msg)
    {
        if (peek().has_value() && peek().value().type == type) {
            return consume();
        }
        else {
            std::cerr << err_msg << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    inline std::optional<Token> try_consume(TokenType type)
    {
        if (peek().has_value() && peek().value().type == type) {
            return consume();
        }
        else {
            return {};
        }
    }

    const std::vector<Token> m_tokens;
    size_t m_index = 0;
    ArenaAllocator m_allocator;
};