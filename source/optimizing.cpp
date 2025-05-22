#include "parser.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stack>
#include <algorithm> // For std::all_of
#include <map>
#include <utility>

using namespace prsr;

namespace prsr {

// Function to format number (removes trailing zeros, converts 0.0 to just 0)
std::string formatNumber(double value) {
    if (value == 0.0) return "0";
    
    std::string str = std::to_string(value);
    // Remove trailing zeros
    str.erase(str.find_last_not_of('0') + 1, std::string::npos);
    // Remove decimal point if it's the last character
    if (str.back() == '.') str.pop_back();
    return str;
}

size_t prsr::findClosingParen(const std::vector<Token>& tokens, size_t start) {
    int count = 1;
    for (size_t i = start + 1; i < tokens.size(); i++) {
        if (tokens[i].value == "(") count++;
        else if (tokens[i].value == ")") count--;
        if (count == 0) return i;
    }
    return tokens.size();
}

std::vector<Token> prsr::processTokens(std::vector<Token> tokens) {
    std::vector<Token> processed;

    for (size_t i = 0; i < tokens.size(); i++) {
        Token token = tokens[i];

        // Handle negative numbers at the beginning or after operators/opening parentheses
        if (token.value == "-" && token.isOperator) {
            bool isNegative = (i == 0) ||
                (i > 0 && (tokens[i - 1].isOperator || tokens[i - 1].value == "("));

            if (isNegative && i + 1 < tokens.size() &&
                (tokens[i + 1].isNumber || std::isdigit(tokens[i + 1].value[0]))) {
                // Combine negative sign with the next number
                std::string negValue = "-" + tokens[i + 1].value;
                processed.push_back({ negValue, false, true, false });
                i++; // Skip the next token as we've combined it
                continue;
            }
        }

        // Identify numbers vs variables
        if (!token.isOperator && token.value != "(" && token.value != ")") {
            bool isNum = true;
            size_t start = 0;
            if (token.value[0] == '-') start = 1;

            for (size_t j = start; j < token.value.length(); j++) {
                if (!std::isdigit(token.value[j]) && token.value[j] != '.') {
                    isNum = false;
                    break;
                }
            }

            if (isNum) {
                processed.push_back({ token.value, false, true, false });
            }
            else {
                processed.push_back({ token.value, false, false, true });
            }
        }
        else {
            processed.push_back(token);
        }
    }

    return processed;
}

std::string prsr::optimizeExpression(std::string& expression) {
    // Tokenize the expression
    std::vector<Token> tokens = tokenize(expression);
    std::vector<Token> optimizedTokens;

    for (size_t i = 0; i < tokens.size(); ++i) {
        // Handle subtraction: just copy the token as is
        if (tokens[i].isOperator && tokens[i].value == "-") {
            optimizedTokens.push_back(tokens[i]);
            continue;
        }
        // Handle division: just copy the token as is
        if (tokens[i].isOperator && tokens[i].value == "/") {
            optimizedTokens.push_back(tokens[i]);
            continue;
        }
        // Default: just copy the token
        optimizedTokens.push_back(tokens[i]);
    }

    // Rebuild the expression from tokens
    std::string result;
    for (const auto& t : optimizedTokens) {
        result += t.value;
    }
    return result;
}

Node* prsr::buildParseTree(const std::string& expr) {
    auto tokens = tokenize(expr);
    tokens = prsr::processTokens(tokens);

    if (tokens.empty()) return nullptr;

    return prsr::buildTreeFromTokens(tokens, 0, tokens.size() - 1);
}

Node* prsr::buildTreeFromTokens(const std::vector<Token>& tokens, size_t start, size_t end) {
    if (start > end) return nullptr;
    if (start == end) {
        const Token& token = tokens[start];
        return new Node(token.value, token.isOperator, token.isNumber, token.isVariable);
    }

    // Find the main operator with lowest precedence (rightmost)
    int minPrec = INT_MAX;
    int opIndex = -1;
    int parenLevel = 0;

    for (int i = end; i >= (int)start; i--) {
        if (tokens[i].value == ")") parenLevel++;
        else if (tokens[i].value == "(") parenLevel--;
        else if (parenLevel == 0 && tokens[i].isOperator) {
            int prec = getPrecedence(tokens[i].value);
            if (prec <= minPrec) {
                minPrec = prec;
                opIndex = i;
            }
        }
    }

    if (opIndex == -1) {
        // No operator found, might be parentheses
        if (tokens[start].value == "(" && tokens[end].value == ")") {
            return prsr::buildTreeFromTokens(tokens, start + 1, end - 1);
        }
        // Single token
        const Token& token = tokens[start];
        return new Node(token.value, token.isOperator, token.isNumber, token.isVariable);
    }

    // Create operator node
    Node* root = new Node(tokens[opIndex].value, true, false, false);
    root->children.push_back(prsr::buildTreeFromTokens(tokens, start, opIndex - 1));
    root->children.push_back(prsr::buildTreeFromTokens(tokens, opIndex + 1, end));

    return root;
}

Node* createBalancedStructureForAllOps(Node* root) {
    if (!root || !root->isOperator) return root;
    std::string op = root->value;
    std::vector<Node*> operands;
    std::function<void(Node*)> collect = [&](Node* node) {
        if (node && node->isOperator && node->value == op) {
            for (Node* child : node->children) collect(child);
        } else if (node) {
            operands.push_back(node);
        }
    };
    collect(root);
    if (operands.size() <= 2) return root;
    // Build balanced tree
    std::vector<Node*> current = operands;
    while (current.size() > 1) {
        std::vector<Node*> next;
        for (size_t i = 0; i < current.size(); i += 2) {
            if (i + 1 < current.size()) {
                Node* newNode = new Node(op, true, false, false);
                newNode->children.push_back(current[i]);
                newNode->children.push_back(current[i + 1]);
                next.push_back(newNode);
            } else {
                next.push_back(current[i]);
            }
        }
        current = next;
    }
    return current[0];
}

Node* prsr::optimizeParallelTree(Node* root) {
    if (!root) return nullptr;
    // First, recursively optimize children
    for (size_t i = 0; i < root->children.size(); i++) {
        root->children[i] = prsr::optimizeParallelTree(root->children[i]);
    }
    // If this is an operator node, try to create parallel structure for any operator
    if (root->isOperator) {
        return createBalancedStructureForAllOps(root);
    }
    return root;
}

Node* prsr::createParallelStructure(Node* root) {
    if (!root || !root->isOperator) return root;

    std::string op = root->value;
    std::vector<Node*> operands;

    // Collect all operands of the same operator type
    prsr::collectOperands(root, op, operands);

    if (operands.size() <= 2) return root;

    // Create balanced tree with maximum width and minimum height
    return buildBalancedTree(operands, op);
}

void prsr::collectOperands(Node* node, const std::string& op, std::vector<Node*>& operands) {
    if (!node) return;

    if (node->isOperator && node->value == op) {
        for (Node* child : node->children) {
            prsr::collectOperands(child, op, operands);
        }
    }
    else {
        operands.push_back(new Node(node->value, node->isOperator, node->isNumber, node->isVariable));
        // Copy children if any
        for (Node* child : node->children) {
            operands.back()->children.push_back(child);
        }
    }
}

Node* prsr::buildBalancedTree(std::vector<Node*>& operands, const std::string& op) {
    if (operands.empty()) return nullptr;
    if (operands.size() == 1) return operands[0];

    // Create tree level by level to minimize height and maximize width
    std::vector<Node*> currentLevel = operands;

    while (currentLevel.size() > 1) {
        std::vector<Node*> nextLevel;

        // Group operands to create maximum width at each level
        for (size_t i = 0; i < currentLevel.size(); i += 2) {
            if (i + 1 < currentLevel.size()) {
                Node* newNode = new Node(op, true, false, false);
                newNode->children.push_back(currentLevel[i]);
                newNode->children.push_back(currentLevel[i + 1]);
                nextLevel.push_back(newNode);
            }
            else {
                nextLevel.push_back(currentLevel[i]);
            }
        }

        currentLevel = nextLevel;
    }

    return currentLevel[0];
}

// Helper: check if all tokens are numbers/operators
bool isAllNumbersAndOperators(const std::vector<prsr::Token>& tokens) {
    for (const auto& t : tokens) {
        if (!t.isNumber && !t.isOperator) return false;
    }
    return !tokens.empty();
}

// Enhanced simplifyVariables: remove +0, -0, combine like terms (a-a, b+0, b-0)
std::string simplifyVariables(const std::vector<prsr::Token>& tokens) {
    std::vector<prsr::Token> out;
    for (size_t i = 0; i < tokens.size(); ++i) {
        // Remove +0 or -0 (e.g., b+0, b-0)
        if (tokens[i].isOperator && (tokens[i].value == "+" || tokens[i].value == "-") &&
            i + 1 < tokens.size() && tokens[i + 1].isNumber && tokens[i + 1].value == "0") {
            ++i;
            continue;
        }
        // Remove 0+ or 0- (e.g., 0+b, 0-b)
        if (tokens[i].isNumber && tokens[i].value == "0" &&
            i + 1 < tokens.size() && tokens[i + 1].isOperator && (tokens[i + 1].value == "+" || tokens[i + 1].value == "-")) {
            continue;
        }
        // Combine a-a -> 0
        if (tokens[i].isVariable && i + 2 < tokens.size() &&
            tokens[i + 1].isOperator && tokens[i + 1].value == "-" &&
            tokens[i + 2].isVariable && tokens[i].value == tokens[i + 2].value) {
            out.push_back(prsr::Token("0", false, true, false));
            i += 2;
            continue;
        }
        out.push_back(tokens[i]);
    }
    std::string result;
    for (auto& t : out) result += t.value;
    return result;
}

// Enhanced removeDivisionByZero: replace a/0 or (expr)/0 with 0
std::string removeDivisionByZero(const std::vector<prsr::Token>& tokens) {
    std::vector<prsr::Token> out;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].isOperator && tokens[i].value == "/" &&
            i + 1 < tokens.size() && tokens[i + 1].isNumber && tokens[i + 1].value == "0") {
            // Replace previous operand and /0 with 0
            if (!out.empty()) out.pop_back();
            out.push_back(prsr::Token("0", false, true, false));
            ++i;
            continue;
        }
        out.push_back(tokens[i]);
    }
    std::string result;
    for (auto& t : out) result += t.value;
    return result;
}

// Helper: simplify multiplication by zero (a*0, 0*a -> 0)
std::string simplifyMultiplicationByZero(const std::vector<Token>& tokens) {
    std::vector<Token> out;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].isOperator && tokens[i].value == "*" &&
            ((i > 0 && tokens[i-1].isNumber && tokens[i-1].value == "0") ||
             (i+1 < tokens.size() && tokens[i+1].isNumber && tokens[i+1].value == "0"))) {
            // Replace the whole product with 0
            if (!out.empty()) out.pop_back();
            out.push_back(Token("0", false, true, false));
            if (i+1 < tokens.size() && tokens[i+1].isNumber && tokens[i+1].value == "0") ++i;
            continue;
        }
        out.push_back(tokens[i]);
    }
    std::string result;
    for (auto& t : out) result += t.value;
    return result;
}

// Додаємо правило спрощення -1*B -> -B
std::string simplifyNegativeMultiplication(const std::vector<Token>& tokens) {
    std::vector<Token> out;
    for (size_t i = 0; i < tokens.size(); ++i) {
        // Перевірка на -1*B
        if (tokens[i].isNumber && tokens[i].value == "-1" &&
            i + 2 < tokens.size() && tokens[i + 1].value == "*" && tokens[i + 1].isOperator &&
            (tokens[i + 2].isVariable || tokens[i + 2].isNumber)) {
            // Замінюємо -1*B на -B
            out.push_back(Token("-" + tokens[i + 2].value, false, 
                         tokens[i + 2].isNumber, tokens[i + 2].isVariable));
            i += 2; // Пропускаємо "*" і "B"
            continue;
        }
        out.push_back(tokens[i]);
    }
    std::string result;
    for (auto& t : out) result += t.value;
    return result;
}

std::string prsr::simplifyParentheses(const std::vector<Token>& tokens) {
    // Special case: if the whole expression is a single parenthesis group, simplify inside
    if (tokens.size() >= 2 && tokens.front().value == "(" && tokens.back().value == ")") {
        std::vector<Token> inner(tokens.begin() + 1, tokens.end() - 1);
        std::string innerStr;
        for (auto& t : inner) innerStr += t.value;
        std::string simplified = simplifyExpression(innerStr);
        // If the result is a number, return it without parentheses
        bool isNum = !simplified.empty() && std::all_of(simplified.begin(), simplified.end(), [](char c){ 
            return (std::isdigit(c) || c == '-' || c == '.'); 
        });
        if (isNum) return simplified;
        return "(" + simplified + ")";
    }
    
    std::vector<Token> out;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].value == "(") {
            size_t j = i + 1, depth = 1;
            std::vector<Token> sub;
            for (; j < tokens.size(); ++j) {
                if (tokens[j].value == "(") depth++;
                if (tokens[j].value == ")") depth--;
                if (depth == 0) break;
                sub.push_back(tokens[j]);
            }
            // Якщо всередині лише числа й оператори — обчислити
            bool onlyNums = true;
            for (auto& t : sub) if (!t.isNumber && !t.isOperator) onlyNums = false;
            if (onlyNums && !sub.empty()) {
                bool ok = false;
                double val = evalSimpleExpr(sub, ok);
                if (ok) {
                    out.push_back(Token(formatNumber(val), false, true, false));
                } else {
                    out.push_back(tokens[i]);
                    out.insert(out.end(), sub.begin(), sub.end());
                    out.push_back(tokens[j]);
                }
            } else {
                // Recursively simplify inside parentheses
                std::string subStr;
                for (auto& t : sub) subStr += t.value;
                std::string simplified = simplifyExpression(subStr);
                out.push_back(Token("(", false, false, false));
                for (auto c : tokenize(simplified)) out.push_back(c);
                out.push_back(Token(")", false, false, false));
            }
            i = j;
        } else {
            out.push_back(tokens[i]);
        }
    }
    // Зібрати назад у рядок
    std::string result;
    for (auto& t : out) result += t.value;
    return result;
}

std::string prsr::simplifyExpression(const std::string& expr) {
    std::string current = expr;
    std::string prev;
    do {
        prev = current;
        auto tokens = tokenize(current);
        // 1. Simplify parentheses (now handles outermost)
        current = simplifyParentheses(tokens);
        tokens = tokenize(current);
        // 2. Simplify multiplication by zero
        current = simplifyMultiplicationByZero(tokens);
        tokens = tokenize(current);
        // 3. Remove division by zero
        current = removeDivisionByZero(tokens);
        tokens = tokenize(current);
        // 4. Simplify variables and remove +0/-0
        current = simplifyVariables(tokens);
        tokens = tokenize(current);
        // 5. Simplify -1*B -> -B
        current = simplifyNegativeMultiplication(tokens);
        tokens = tokenize(current);
        // 6. If all tokens are numbers/operators, evaluate
        if (isAllNumbersAndOperators(tokens)) {
            bool ok = false;
            double val = evalSimpleExpr(tokens, ok);
            if (ok) {
                current = formatNumber(val);
            }
        }
    } while (current != prev);
    return current;
}

double evalSimpleExpr(const std::vector<Token>& tokens, bool& ok) {
    std::vector<double> output;
    std::vector<char> ops;
    std::map<char, int> precedence = {{'+', 1}, {'-', 1}, {'*', 2}, {'/', 2}};
    std::map<char, bool> leftAssoc = {{'+', true}, {'-', true}, {'*', true}, {'/', true}};

    // Shunting Yard: convert to RPN
    std::vector<std::string> rpn;
    std::stack<std::string> opStack;
    for (const auto& t : tokens) {
        if (t.isNumber) {
            rpn.push_back(t.value);
        } else if (t.isOperator) {
            while (!opStack.empty()) {
                char top = opStack.top()[0];
                if (precedence[top] > precedence[t.value[0]] ||
                    (precedence[top] == precedence[t.value[0]] && leftAssoc[t.value[0]])) {
                    rpn.push_back(opStack.top());
                    opStack.pop();
                } else {
                    break;
                }
            }
            opStack.push(t.value);
        }
    }
    while (!opStack.empty()) {
        rpn.push_back(opStack.top());
        opStack.pop();
    }

    // Evaluate RPN
    std::stack<double> evalStack;
    for (const auto& s : rpn) {
        if (s == "+" || s == "-" || s == "*" || s == "/") {
            if (evalStack.size() < 2) { ok = false; return 0; }
            double b = evalStack.top(); evalStack.pop();
            double a = evalStack.top(); evalStack.pop();
            if (s == "+") evalStack.push(a + b);
            else if (s == "-") evalStack.push(a - b);
            else if (s == "*") evalStack.push(a * b);
            else if (s == "/") evalStack.push(b == 0 ? 0 : a / b);
        } else {
            evalStack.push(std::stod(s));
        }
    }
    ok = (evalStack.size() == 1);
    return ok ? evalStack.top() : 0;
}

// Допоміжна функція: чи є вузол простим (змінна або число)
bool isSimple(Node* node) {
    return node && !node->isOperator;
}

// Рекурсивно будує вираз з правильними дужками і знаками
void expandMinusSmart(Node* node, int sign, std::string& out) {
    if (!node) return;
    if (node->isOperator) {
        if (node->value == "+") {
            for (size_t i = 0; i < node->children.size(); ++i) {
                if (i > 0) out += (sign == 1 ? "+" : "-");
                expandMinusSmart(node->children[i], sign, out);
            }
        } else if (node->value == "-") {
            expandMinusSmart(node->children[0], sign, out);
            out += (sign == 1 ? "-" : "+");
            expandMinusSmart(node->children[1], -sign, out);
        } else {
            // *, / — залишаємо дужки, якщо sign == -1
            if (sign == -1) out += "-(";
            else if (!isSimple(node)) out += "(";
            expandMinusSmart(node->children[0], 1, out);
            out += node->value;
            expandMinusSmart(node->children[1], 1, out);
            if (sign == -1 || !isSimple(node)) out += ")";
        }
    } else {
        if (sign == -1) out += "-";
        out += node->value;
    }
}

std::string prsr::flattenExpandMinus(Node* root) {
    std::string result;
    expandMinusSmart(root, 1, result);
    return result;
}

Node* prsr::cloneSubtree(Node* node) {
    if (!node) return nullptr;
    Node* copy = new Node(node->value, node->isOperator, node->isNumber, node->isVariable);
    for (auto* child : node->children) {
        copy->children.push_back(cloneSubtree(child));
    }
    return copy;
}

Node* prsr::applyDistributive(Node* node) {
    if (!node) return nullptr;
    // Рекурсивно обробити дітей
    for (size_t i = 0; i < node->children.size(); ++i) {
        node->children[i] = applyDistributive(node->children[i]);
    }
    // Якщо це множення і один з дітей — додавання
    if (node->isOperator && node->value == "*") {
        Node* left = node->children[0];
        Node* right = node->children[1];
        if (left && left->isOperator && left->value == "+") {
            Node* newPlus = new Node("+", true, false, false);
            for (auto* lchild : left->children) {
                Node* mul = new Node("*", true, false, false);
                mul->children.push_back(cloneSubtree(lchild));
                mul->children.push_back(cloneSubtree(right));
                newPlus->children.push_back(applyDistributive(mul));
            }
            return newPlus;
        }
        if (right && right->isOperator && right->value == "+") {
            Node* newPlus = new Node("+", true, false, false);
            for (auto* rchild : right->children) {
                Node* mul = new Node("*", true, false, false);
                mul->children.push_back(cloneSubtree(left));
                mul->children.push_back(cloneSubtree(rchild));
                newPlus->children.push_back(applyDistributive(mul));
            }
            return newPlus;
        }
    }
    return node;
}

Node* prsr::applyAssociative(Node* node) {
    if (!node) return nullptr;
    // Рекурсивно обробити дітей
    for (size_t i = 0; i < node->children.size(); ++i) {
        node->children[i] = applyAssociative(node->children[i]);
    }
    // Якщо це + або * — збираємо всі піддерева з таким же оператором у плоский список
    if (node->isOperator && (node->value == "+" || node->value == "*")) {
        std::vector<Node*> flat;
        std::function<void(Node*)> collect = [&](Node* n) {
            if (n && n->isOperator && n->value == node->value) {
                for (auto* c : n->children) collect(c);
            } else if (n) {
                flat.push_back(n);
            }
        };
        collect(node);
        // Побудувати збалансоване дерево
        std::function<Node*(int,int)> build = [&](int l, int r) -> Node* {
            if (l == r) return flat[l];
            int m = (l + r) / 2;
            Node* root = new Node(node->value, true, false, false);
            root->children.push_back(build(l, m));
            root->children.push_back(build(m+1, r));
            return root;
        };
        if (!flat.empty())
            return build(0, flat.size()-1);
    }
    return node;
}

// Факторизація: a*b + a*c → a*(b+c)
Node* prsr::factorize(Node* node) {
    if (!node) return nullptr;
    // Рекурсивно обробити дітей
    for (size_t i = 0; i < node->children.size(); ++i) {
        node->children[i] = factorize(node->children[i]);
    }
    // Працюємо лише для додавання
    if (node->isOperator && node->value == "+") {
        // Зберемо множники для кожного доданка
        std::vector<std::vector<Node*>> factors;
        for (auto* child : node->children) {
            if (child->isOperator && child->value == "*") {
                factors.push_back(child->children);
            } else {
                factors.push_back({child});
            }
        }
        // Пошук множників, які зустрічаються у >=2 доданках
        std::map<std::string, int> freq;
        for (const auto& fs : factors) {
            for (auto* f : fs) freq[f->value]++;
        }
        // Знайти найчастіший множник, який зустрічається у >=2 доданках
        std::string best;
        int bestCount = 1;
        for (const auto& p : freq) {
            if (p.second > bestCount) {
                best = p.first;
                bestCount = p.second;
            }
        }
        if (bestCount > 1) {
            // Винести best за дужки
            Node* newMul = new Node("*", true, false, false);
            newMul->children.push_back(new Node(best, false, false, true));
            Node* newPlus = new Node("+", true, false, false);
            std::vector<Node*> restTerms;
            for (auto& fs : factors) {
                bool found = false;
                std::vector<Node*> rest;
                for (auto* f : fs) {
                    if (f->value == best && !found) found = true;
                    else rest.push_back(prsr::cloneSubtree(f));
                }
                if (found) {
                    if (rest.empty()) {
                        rest.push_back(new Node("1", false, true, false));
                    }
                    if (rest.size() == 1) {
                        newPlus->children.push_back(rest[0]);
                    } else {
                        Node* mul = new Node("*", true, false, false);
                        for (auto* r : rest) mul->children.push_back(r);
                        newPlus->children.push_back(mul);
                    }
                } else {
                    // Доданки без best залишаємо для подальшої факторизації
                    if (fs.size() == 1) restTerms.push_back(prsr::cloneSubtree(fs[0]));
                    else {
                        Node* mul = new Node("*", true, false, false);
                        for (auto* r : fs) mul->children.push_back(prsr::cloneSubtree(r));
                        restTerms.push_back(mul);
                    }
                }
            }
            newMul->children.push_back(factorize(newPlus));
            if (!restTerms.empty()) {
                Node* restPlus = new Node("+", true, false, false);
                for (auto* t : restTerms) restPlus->children.push_back(t);
                // Рекурсивно факторизуємо залишок
                Node* result = new Node("+", true, false, false);
                result->children.push_back(newMul);
                result->children.push_back(factorize(restPlus));
                return result;
            } else {
                return newMul;
            }
        }
    }
    return node;
}

} // namespace prsr


