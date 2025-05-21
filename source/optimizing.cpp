#include "parser.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stack>

using namespace prsr;

size_t prsr::findClosingParen(const std::vector<Token>& tokens, size_t start) {
    size_t depth = 0;
    for (size_t i = start; i < tokens.size(); ++i) {
        if (tokens[i].value == "(") depth++;
        else if (tokens[i].value == ")") depth--;
        if (depth == 0) return i;
    }
    return tokens.size();
}

std::vector<Token> prsr::processTokens(std::vector<Token> tokens) {
    bool changed = true;
    std::vector<Token> result = tokens;
    std::string currentOp = "+"; // Оголошуємо поза циклами

    while (changed) {
        changed = false;
        std::vector<Token> newResult;

        for (size_t i = 0; i < result.size(); ++i) {
            if (i + 2 < result.size() && result[i].isNumber && result[i].value == "0" &&
                result[i + 1].isOperator && result[i + 1].value == "*") {
                if (result[i + 2].value == "(") {
                    size_t closing = findClosingParen(result, i + 2);
                    if (closing != result.size()) {
                        newResult.push_back({ "0", false, true, false });
                        i = closing;
                        changed = true;
                        continue;
                    }
                }
                else if (result[i + 2].isVariable || result[i + 2].isNumber) {
                    size_t j = i + 2;
                    while (j + 1 < result.size() && result[j].isVariable && result[j + 1].isOperator && result[j + 1].value == "*") {
                        j += 2;
                    }
                    if (j < result.size() && (result[j].isVariable || result[j].isNumber)) {
                        newResult.push_back({ "0", false, true, false });
                        i = j;
                        changed = true;
                        continue;
                    }
                    else if (j == i + 2) {
                        newResult.push_back({ "0", false, true, false });
                        i = j;
                        changed = true;
                        continue;
                    }
                }
            }
            newResult.push_back(result[i]);
        }

        result = newResult;
        newResult.clear();

        int constantSum = 0;
        bool hasConstant = false;

        for (size_t i = 0; i < result.size(); ++i) {
            if (result[i].isNumber) {
                hasConstant = true;
                int num = std::stoi(result[i].value);
                if (currentOp == "+") constantSum += num;
                else if (currentOp == "-") constantSum -= num;

                if (i + 1 >= result.size() || (result[i + 1].isOperator && (result[i + 1].value == "+" || result[i + 1].value == "-"))) {
                    if (constantSum != 0 || !hasConstant) {
                        if (!newResult.empty() && (newResult.back().value == "+" || newResult.back().value == "-")) {
                            newResult.push_back({ std::to_string(constantSum), false, true, false });
                        }
                        else {
                            if (constantSum < 0) {
                                newResult.push_back({ "-", true, false, false });
                                newResult.push_back({ std::to_string(-constantSum), false, true, false });
                            }
                            else {
                                newResult.push_back({ std::to_string(constantSum), false, true, false });
                            }
                        }
                    }
                    constantSum = 0;
                    hasConstant = false;
                }
            }
            else {
                if (result[i].isOperator && (result[i].value == "+" || result[i].value == "-")) {
                    currentOp = result[i].value;
                }
                newResult.push_back(result[i]);
            }
        }

        if (newResult != result) {
            changed = true;
            result = newResult;
        }
    }

    // Use std::unordered_map instead of std::map
    std::unordered_map<std::string, int> termCount;

    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i].isVariable) {
            if (currentOp == "+") termCount[result[i].value]++;
            else if (currentOp == "-") termCount[result[i].value]--;
        }
        else if (result[i].isOperator) {
            currentOp = result[i].value;
        }
    }

    std::vector<Token> simplified;
    for (const auto& term : termCount) {
        if (term.second != 0) {
            if (!simplified.empty()) {
                simplified.push_back({ term.second > 0 ? "+" : "-", true, false, false });
            }
            else if (term.second < 0) {
                simplified.push_back({ "-", true, false, false });
            }
            if (std::abs(term.second) != 1) {
                simplified.push_back({ std::to_string(std::abs(term.second)), false, true, false });
                simplified.push_back({ "*", true, false, false });
            }
            simplified.push_back({ term.first, false, false, true });
        }
    }

    int constantSum = 0;
    bool hasConstant = false;

    for (const auto& token : result) {
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
        if (!simplified.empty()) {
            simplified.push_back({ constantSum > 0 ? "+" : "-", true, false, false });
        }
        else if (constantSum < 0) {
            simplified.push_back({ "-", true, false, false });
        }
        simplified.push_back({ std::to_string(std::abs(constantSum)), false, true, false });
    }
    else if (simplified.empty() && constantSum == 0) {
        simplified.push_back({ "0", false, true, false });
    }

    return simplified;
}

std::string prsr::optimizeExpression(std::string& expression) {
    std::vector<Token> tokens = tokenize(expression);
    std::vector<Token> simplified = processTokens(tokens);

    std::string result;
    for (const auto& token : simplified) {
        result += token.value;
    }
    prsr::optimizedExpression = result;
    return result;
}

Node* prsr::buildParseTree(const std::string& expr) {
    std::vector<Token> tokens = tokenize(expr);
    std::stack<Node*> values;
    std::stack<std::string> ops;

    for (size_t i = 0; i < tokens.size(); i++) {
        const Token& token = tokens[i];

        if (token.value == "(") {
            ops.push(token.value);
        }
        else if (token.value == ")") {
            while (!ops.empty() && ops.top() != "(") {
                Node* node = new Node(ops.top(), true);
                ops.pop();

                if (!values.empty()) {
                    node->children.insert(node->children.begin(), values.top());
                    values.pop();
                }
                if (!values.empty()) {
                    node->children.insert(node->children.begin(), values.top());
                    values.pop();
                }

                values.push(node);
            }
            if (!ops.empty()) ops.pop(); // Remove '('
        }
        else if (token.isOperator) {
            while (!ops.empty() && ops.top() != "(" &&
                getPrecedence(ops.top()) >= getPrecedence(token.value)) {
                Node* node = new Node(ops.top(), true);
                ops.pop();

                if (!values.empty()) {
                    node->children.insert(node->children.begin(), values.top());
                    values.pop();
                }
                if (!values.empty()) {
                    node->children.insert(node->children.begin(), values.top());
                    values.pop();
                }

                values.push(node);
            }
            ops.push(token.value);
        }
        else { // Number or variable
            values.push(new Node(token.value, false));
        }
    }

    // Process remaining operators
    while (!ops.empty()) {
        Node* node = new Node(ops.top(), true);
        ops.pop();

        if (!values.empty()) {
            node->children.insert(node->children.begin(), values.top());
            values.pop();
        }
        if (!values.empty()) {
            node->children.insert(node->children.begin(), values.top());
            values.pop();
        }

        values.push(node);
    }

    return values.empty() ? nullptr : values.top();
}

Node* prsr::optimizeParallelTree(Node* root) {
    if (!root) return nullptr;

    // First optimize children
    for (size_t i = 0; i < root->children.size(); ++i) {
        root->children[i] = optimizeParallelTree(root->children[i]);
    }

    if (root->isOperator && root->children.size() == 2) {
        Node* left = root->children[0];
        Node* right = root->children[1];

        // Handle multiplication by zero
        if (root->value == "*") {
            if (left->value == "0" && !left->isOperator) {
                delete right;
                return left;
            }
            if (right->value == "0" && !right->isOperator) {
                delete left;
                return right;
            }
        }

        // Handle addition/subtraction with zero
        if (root->value == "+" || root->value == "-") {
            if (left->value == "0" && !left->isOperator) {
                if (root->value == "+") {
                    delete left;
                    return right;
                }
            }
            if (right->value == "0" && !right->isOperator) {
                delete right;
                return left;
            }
        }

        // Combine constants
        if (left->isNumber && right->isNumber) {
            int leftVal = std::stoi(left->value);
            int rightVal = std::stoi(right->value);
            int result = 0;

            if (root->value == "+") result = leftVal + rightVal;
            else if (root->value == "-") result = leftVal - rightVal;
            else if (root->value == "*") result = leftVal * rightVal;
            else if (root->value == "/" && rightVal != 0) result = leftVal / rightVal;

            delete left;
            delete right;
            return new Node(std::to_string(result), false);
        }
    }

    return root;
}

std::string prsr::treeToString(Node* root) {
    if (!root) return "";

    std::stringstream ss;
    if (!root->isOperator) {
        ss << root->value;
    }
    else {
        ss << "(";
        for (size_t i = 0; i < root->children.size(); ++i) {
            if (i > 0) ss << " " << root->value << " ";
            ss << treeToString(root->children[i]);
        }
        ss << ")";
    }
    return ss.str();
}