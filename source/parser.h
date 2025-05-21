#pragma once

#include <string>
#include <vector>

namespace prsr {
    struct Node {
        std::string value;
        bool isOperator;
        bool isNumber;    // Add this
        bool isVariable;  // Add this
        std::vector<Node*> children;

        Node(const std::string& val, bool op, bool num = false, bool var = false)
            : value(val), isOperator(op), isNumber(num), isVariable(var), children() {}

        ~Node() {
            for (Node* child : children) {
                delete child;
            }
        }
    };

    struct Token {
        std::string value;
        bool isOperator;
        bool isNumber;
        bool isVariable;

        Token(const std::string& val, bool op, bool num, bool var)
            : value(val), isOperator(op), isNumber(num), isVariable(var) {}

        // Define operator== to compare two Token objects
        bool operator==(const Token& other) const {
            return value == other.value &&
                isOperator == other.isOperator &&
                isNumber == other.isNumber &&
                isVariable == other.isVariable;
        }
    };


    extern std::vector<std::string> errors;
    extern char expression[256];
    extern std::string correctedExpression;
    extern std::string optimizedExpression;

    // Helper functions
    bool isOperator(char c);
    Token createToken(const std::string& val);
    int getPrecedence(const std::string& op);

    std::vector<std::string> checkExpression(const char* expr);
    void displayErrors(const std::vector<std::string>& errors);
    std::string correctExpression(char* expr, const std::vector<std::string>& errors);
    std::string optimizeExpression(std::string& expression);
    Node* buildParseTree(const std::string& expr);
    Node* optimizeParallelTree(Node* root);
    std::string treeToString(Node* root);

    inline std::vector<Token> tokenize(const std::string& expr) {
        std::vector<Token> tokens;
        std::string current;
        bool expectNegative = true;

        for (size_t i = 0; i < expr.length(); i++) {
            char c = expr[i];
            if (std::isspace(c)) continue;

            if (c == '(' || c == ')') {
                if (!current.empty()) {
                    tokens.push_back({ current, false, false, true });
                    current.clear();
                }
                tokens.push_back({ std::string(1, c), false, false, false });
                expectNegative = (c == '(');
            }
            else if (isOperator(c)) {
                if (!current.empty()) {
                    tokens.push_back({ current, false, false, true });
                    current.clear();
                }
                tokens.push_back({ std::string(1, c), true, false, false });
                expectNegative = true;
            }
            else if (c >= 'A' && c <= 'Z') {  
                current += c;
                expectNegative = false;
            }
        }

        if (!current.empty()) {
            tokens.push_back({ current, false, false, true });
        }

        return tokens;
    }

    size_t findClosingParen(const std::vector<Token>& tokens, size_t start);
    std::vector<Token> processTokens(std::vector<Token> tokens);
}
