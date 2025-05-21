#include "parser.h"
#include <regex>
#include <string>
#include <vector>
#include <stack>
#include <iostream>
#include <cctype>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

using namespace prsr;
// List of available functions
const std::vector<std::string> functions = { "SIN", "COS", "TAN", "SQRT" };

bool prsr::isOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/';
}

prsr::Token prsr::createToken(const std::string& val) {
    bool isNum = false;
    bool isVar = false;

    if (val.empty()) {
        return { "", false, false, false };
    }

    // Check if it's a number (including negative numbers)
    size_t start = 0;
    if (val[0] == '-') {
        start = 1;
        if (val.size() == 1) {
            return { val, true, false, false }; // Just a '-' operator
        }
    }

    isNum = true;
    for (size_t i = start; i < val.size(); ++i) {
        if (!std::isdigit(val[i])) {
            isNum = false;
            break;
        }
    }

    // If not a number, check if it's a valid variable (single letter)
    if (!isNum) {
        isVar = (val.size() == 1 && std::isalpha(val[0]));
    }

    return { val, false, isNum, isVar };
}

int prsr::getPrecedence(const std::string& op) {
    if (op == "*" || op == "/") return 2;
    if (op == "+" || op == "-") return 1;
    return 0;
}

std::vector<std::string> prsr::checkExpression(const char* expr) {
    int parenthesesCount = 0;
    bool lastWasOperator = true;
    bool lastWasDecimal = false;
    bool inNumber = false;
    bool lastWasOpeningParenthesis = false;
    bool lastWasNegativeSign = false;
    bool lastWasOperand = false;

    std::cout << "Current expression: " << expr << std::endl;
    size_t len = strlen(expr);

    if (len > 0 && (expr[0] == '+' || expr[0] == '*' || expr[0] == '/')) {
        prsr::errors.push_back("Position 0: expression starts with an invalid operator '" + std::string(1, expr[0]) + "'");
    }

    auto tokens = tokenize(expr);
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].value == "/" && i + 1 < tokens.size()) {
            if (tokens[i + 1].isNumber && tokens[i + 1].value == "0") {
                prsr::errors.push_back("Position " + std::to_string(i + 1) + ": division by zero detected");
            }
        }
    }

    for (size_t i = 0; i < len; ++i) {
        char current = expr[i];

        if (!std::isdigit(current) && !std::isalpha(current) && current != '+' && current != '-' &&
            current != '*' && current != '/' && current != '(' && current != ')' && current != '.') {
            prsr::errors.push_back("Position " + std::to_string(i) + ": invalid character '" + std::string(1, current) + "'");
            continue;
        }

        if (i > 0 && std::isdigit(expr[i - 1]) && (std::isalpha(current) || current == '(')) {
            prsr::errors.push_back("Position " + std::to_string(i) + ": missing operator between number and variable/function");
        }

        if (std::isalpha(current)) {
            std::string funcName;
            size_t j = i;
            while (j < len && std::isalpha(expr[j])) {
                funcName += expr[j];
                ++j;
            }
            if (std::find(functions.begin(), functions.end(), funcName) != functions.end()) {
                i = j - 1;
                if (j >= len || expr[j] != '(') {
                    prsr::errors.push_back("Position " + std::to_string(i) + ": function '" + funcName + "' missing opening parenthesis");
                }
                else {
                    lastWasOperator = false;
                    lastWasOperand = false;
                }
                continue;
            }
        }

        if (std::isalpha(current)) {
            std::string varName;
            size_t j = i;
            while (j < len && std::isalpha(expr[j])) {
                varName += expr[j];
                ++j;
            }
            if (varName.size() > 1 || (j < len && std::isdigit(expr[j]))) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": invalid variable name '" + varName + "'");
                i = j - 1;
                continue;
            }
            lastWasOperator = false;
            lastWasOperand = true;
            lastWasOpeningParenthesis = false;
            continue;
        }

        if (current == '+' || current == '-' || current == '*' || current == '/') {
            if (i == 0 && current == '-') {
                lastWasOperator = true;
                lastWasOperand = false;
                lastWasNegativeSign = true;
                continue;
            }
            if (current == '-' && lastWasNegativeSign) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": consecutive negative signs '--' are not allowed");
                continue;
            }
            if (lastWasOpeningParenthesis && current != '-') {
                prsr::errors.push_back("Position " + std::to_string(i) + ": invalid operator '" + std::string(1, current) + "' directly after opening parenthesis");
                continue;
            }
            if (current == '-' && lastWasOpeningParenthesis) {
                lastWasOperator = true;
                lastWasOperand = false;
                lastWasNegativeSign = true;
                continue;
            }
            if (lastWasOperator && !(current == '-' && lastWasOpeningParenthesis)) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": operator '" + std::string(1, current) + "' after another operator");
            }
            if (!lastWasOperand && !lastWasNegativeSign && !lastWasOpeningParenthesis) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": operator '" + std::string(1, current) + "' without preceding operand");
            }
            lastWasOperator = true;
            lastWasOperand = false;
            lastWasDecimal = false;
            inNumber = false;
            lastWasOpeningParenthesis = false;
            lastWasNegativeSign = (current == '-');
            continue;
        }

        if (current == '.') {
            if (lastWasDecimal) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": second decimal point in the number");
            }
            else if (!inNumber) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": decimal point without a number before it");
            }
            else if (i + 1 < len && !std::isdigit(expr[i + 1])) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": decimal point without a number after it");
            }
            lastWasDecimal = true;
            inNumber = true;
            lastWasOperator = false;
            lastWasOperand = true;
            lastWasOpeningParenthesis = false;
            continue;
        }

        if (std::isdigit(current)) {
            inNumber = true;
            lastWasOperator = false;
            lastWasOperand = true;
            lastWasOpeningParenthesis = false;
            continue;
        }

        if (current == '(') {
            parenthesesCount++;
            lastWasOpeningParenthesis = true;
            lastWasOperator = false;
            lastWasOperand = false;
            lastWasDecimal = false;
            inNumber = false;
            lastWasNegativeSign = false;
            continue;
        }
        else if (current == ')') {
            if (lastWasOpeningParenthesis) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": empty parentheses detected");
            }
            else if (lastWasOperator) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": closing parenthesis after an operator");
            }
            if (parenthesesCount == 0) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": extra closing parenthesis");
            }
            else {
                parenthesesCount--;
            }
            lastWasOperator = false;
            lastWasOperand = true;
            lastWasDecimal = false;
            lastWasOpeningParenthesis = false;
            lastWasNegativeSign = false;
            inNumber = false;
            continue;
        }
    }

    if (parenthesesCount > 0) {
        prsr::errors.push_back("Missing closing parenthesis: " + std::to_string(parenthesesCount));
    }

    if (lastWasOperator && len > 0) {
        prsr::errors.push_back("Position " + std::to_string(len - 1) + ": end of expression after an operator, expected a variable or number");
    }

    return prsr::errors;
}

void prsr::displayErrors(const std::vector<std::string>& errors) {
    if (errors.empty()) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "? The expression is correct");
    }
    else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Errors found:");
        for (const auto& error : errors) {
            ImGui::BulletText("%s", error.c_str());
        }
    }
}

std::string prsr::correctExpression(char* expr, const std::vector<std::string>& errors) {
    std::string result;
    std::string currentExpr = expr;
    int parenthesis = 0;
    bool isModified = false;
    bool hasError = true;
    int maxIterations = 10;
    int iteration = 0;

    std::vector<std::string> currentErrors = errors;

    while (hasError && iteration < maxIterations) {
        std::cout << "Iteration " << iteration << " - Current expression length: " << currentExpr.length() << std::endl;
        for (const auto& error : currentErrors) {
            std::cout << "Error: " << error << std::endl;
        }

        parenthesis = 0;
        for (const auto& error : currentErrors) {
            if (error.find("Missing closing parenthesis:") != std::string::npos) {
                std::regex numRegex(R"((\d+))");
                std::smatch match;
                if (std::regex_search(error, match, numRegex)) {
                    parenthesis = std::stoi(match.str(0));
                }
            }
        }

        result = "";
        for (size_t i = 0; i < currentExpr.length(); ++i) {
            isModified = false;
            size_t positionOffset = result.length() - i;

            for (const auto& error : currentErrors) {
                size_t posStart = error.find("Position ");
                if (posStart != std::string::npos) {
                    size_t posEnd = error.find(':', posStart + 9);
                    size_t errorPos = std::stoi(error.substr(posStart + 9, posEnd - (posStart + 9)));
                    if (errorPos + positionOffset == i) {
                        if (error.find("invalid character") != std::string::npos) {
                            isModified = true;
                        }
                        else if (error.find("starts with an invalid") != std::string::npos) {
                            result += '0';
                            isModified = true;
                        }
                        else if (error.find("invalid variable name") != std::string::npos) {
                            result += '0';
                            while (i < currentExpr.length() && (std::isalpha(currentExpr[i]) || std::isdigit(currentExpr[i]))) {
                                ++i;
                            }
                            --i;
                            isModified = true;
                        }
                        else if (error.find("consecutive negative signs") != std::string::npos ||
                            error.find("directly after opening parenthesis") != std::string::npos ||
                            error.find("after another operator") != std::string::npos ||
                            error.find("second decimal point in the number") != std::string::npos ||
                            error.find("extra closing parenthesis") != std::string::npos) {
                            isModified = true;
                        }
                        else if (error.find("decimal point without a number") != std::string::npos ||
                            error.find("after an operator") != std::string::npos) {
                            result += '0';
                            isModified = true;
                        }
                        else if (error.find("empty parentheses detected") != std::string::npos) {
                            result += "0";
                            isModified = true;
                        }
                        else if (error.find("end of expression after an operator") != std::string::npos) {
                            result += currentExpr[i];
                            result += '0';
                            isModified = true;
                        }
                        else if (error.find("missing operator between number and variable/function") != std::string::npos) {
                            result += '*';
                            isModified = true;
                        }
                    }
                }
            }
            if (!isModified) {
                result += currentExpr[i];
            }
            std::cout << "Current expression: " << result << " at position: " << i << std::endl;
        }

        for (int k = 0; k < parenthesis; k++) {
            result += ')';
        }

        currentExpr = result;
        prsr::errors.clear();
        currentErrors = prsr::checkExpression(currentExpr.c_str());
        hasError = !currentErrors.empty();
        iteration++;
    }

    if (iteration >= maxIterations) {
        std::cout << "Warning: Maximum iterations reached, possible infinite loop detected." << std::endl;
    }

    prsr::correctedExpression = currentExpr;
    return currentExpr;
}