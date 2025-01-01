#include "parser.h"
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <cctype>


// Helper function to check if character is operator
bool isOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/';
}

struct Token {
    std::string value;
    bool isOperator;
    bool isNumber;
    bool isVariable;
};

// Tokenize the expression into tokens
std::vector<Token> tokenize(const std::string& expr) {
    std::vector<Token> tokens;
    std::string current;
    bool expectNegative = true;  // true at start and after operators

    for (size_t i = 0; i < expr.length(); i++) {
        if (std::isspace(expr[i])) continue;

        if (expr[i] == '(' || expr[i] == ')') {
            if (!current.empty()) {
                bool isNum = std::isdigit(current[0]);
                tokens.push_back({ current, false, isNum, !isNum });
                current.clear();
            }
            tokens.push_back({ std::string(1, expr[i]), false, false, false });
            expectNegative = true;
        }
        else if (isOperator(expr[i])) {
            if (!current.empty()) {
                bool isNum = std::isdigit(current[0]);
                tokens.push_back({ current, false, isNum, !isNum });
                current.clear();
            }
            // Handle negative numbers
            if (expr[i] == '-' && expectNegative) {
                current = "-";
            }
            else {
                tokens.push_back({ std::string(1, expr[i]), true, false, false });
                expectNegative = true;
            }
        }
        else {
            current += expr[i];
            expectNegative = false;
        }
    }

    if (!current.empty()) {
        bool isNum = std::isdigit(current[0]) || (current[0] == '-' && std::isdigit(current[1]));
        tokens.push_back({ current, false, isNum, !isNum });
    }

    return tokens;
}

// Process tokens to handle identical terms and simplifications
std::vector<Token> processTokens(std::vector<Token> tokens) {
    // Handle identical terms cancellation
    std::map<std::string, int> termCount;
    std::vector<Token> result;
    std::string currentOp = "+";

    // First pass: count terms
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].isVariable) {
            if (currentOp == "+") termCount[tokens[i].value]++;
            else if (currentOp == "-") termCount[tokens[i].value]--;
        }
        else if (tokens[i].isOperator) {
            currentOp = tokens[i].value;
        }
    }

    // Second pass: handle numbers and remaining terms
    for (const auto& term : termCount) {
        if (term.second != 0) {
            if (!result.empty()) {
                result.push_back({ term.second > 0 ? "+" : "-", true, false, false });
            }
            else if (term.second < 0) {
                result.push_back({ "-", true, false, false });
            }
            if (std::abs(term.second) != 1) {
                result.push_back({ std::to_string(std::abs(term.second)), false, true, false });
                result.push_back({ "*", true, false, false });
            }
            result.push_back({ term.first, false, false, true });
        }
    }

    // Handle constant evaluation
    int constantSum = 0;
    bool hasConstant = false;
    currentOp = "+";

    for (const auto& token : tokens) {
        if (token.isNumber) {
            hasConstant = true;
            int num = std::stoi(token.value);
            if (currentOp == "+") constantSum += num;
            else if (currentOp == "-") constantSum -= num;
        }
        else if (token.isOperator) {
            currentOp = token.value;
        }
    }

    if (hasConstant && constantSum != 0) {
        if (!result.empty()) {
            result.push_back({ constantSum > 0 ? "+" : "-", true, false, false });
        }
        else if (constantSum < 0) {
            result.push_back({ "-", true, false, false });
        }
        result.push_back({ std::to_string(std::abs(constantSum)), false, true, false });
    }

    return result;
}

// Helper function to find matching closing parenthesis
size_t findClosingParen(const std::vector<Token>& tokens, size_t start) {
    int count = 1;
    for (size_t i = start + 1; i < tokens.size(); i++) {
        if (tokens[i].value == "(") count++;
        if (tokens[i].value == ")") count--;
        if (count == 0) return i;
    }
    return tokens.size();
}

// Process expression with parentheses
std::vector<Token> processWithParentheses(const std::vector<Token>& tokens) {
    std::vector<Token> result;

    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].value == "(") {
            size_t closing = findClosingParen(tokens, i);
            if (closing == tokens.size()) continue;

            // Extract and process subexpression
            std::vector<Token> subexpr(tokens.begin() + i + 1, tokens.begin() + closing);
            auto processed = processTokens(processWithParentheses(subexpr));

            // If processed result is single number/variable or empty, remove parentheses
            if (processed.size() <= 1) {
                result.insert(result.end(), processed.begin(), processed.end());
            }
            else {
                result.push_back({ "(", false, false, false });
                result.insert(result.end(), processed.begin(), processed.end());
                result.push_back({ ")", false, false, false });
            }

            i = closing;
        }
        else if (tokens[i].value != ")") {
            result.push_back(tokens[i]);
        }
    }

    // If no parentheses or after processing all parentheses
    if (result.size() > 0 && result[0].value != "(") {
        return processTokens(result);
    }

    return result;
}

std::string prsr::optimizeExpression(std::string& expr) {
    auto tokens = tokenize(expr);
    auto processed = processWithParentheses(tokens);

    // Convert back to string
    std::string result;
    for (size_t i = 0; i < processed.size(); i++) {
        const auto& token = processed[i];

        // Add space before operators (except negative sign at start)
        if (i > 0 && token.isOperator && token.value != "-") {
            result += " ";
        }

        result += token.value;

        // Add space after non-operators and non-parentheses
        if (!token.isOperator && token.value != "(" && token.value != ")") {
            result += " ";
        }
    }

    return result.empty() ? "0" : result;
}