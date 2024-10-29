#include "parser.h"
#include <string>
#include <vector>
#include <iostream>
#include <cctype>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

// List of available functions
const std::vector<std::string> functions = { "SIN", "COS", "TAN", "SQRT" };

std::vector<std::string> prsr::checkExpression(char* expr) {
    int parenthesesCount = 0; // Counter for open parentheses
    bool lastWasOperator = false; // Start as true to allow a variable first
    bool lastWasDecimal = false; // Track if the last number had a decimal point
    bool inNumber = false; // Track if we're inside a number
    bool lastWasOpeningParenthesis = false; // Track if the last character was an opening parenthesis
    bool lastWasNegativeSign = false; // Track if the last character was a negative sign

    std::cout << "Current expression: " << expr << std::endl;
    size_t len = strlen(expr);

    for (int i = 0; i < len; ++i) {
        char current = expr[i];

        // Check for invalid characters
        if (!std::isdigit(current) && !std::isalpha(current) && current != '+' && current != '-' &&
            current != '*' && current != '/' && current != '(' && current != ')' && current != '.') {
            prsr::errors.push_back("Position " + std::to_string(i) + ": invalid character '" + std::string(1, current) + "'");
            continue;
        }

        // Check for functions
        if (std::isalpha(current)) {
            std::string funcName;
            int j = i;
            // Collect letters to form potential function name
            while (j < len && std::isalpha(expr[j])) {
                funcName += expr[j];
                ++j;
            }

            // Check if collected name is a valid function
            if (std::find(functions.begin(), functions.end(), funcName) != functions.end()) {
                i = j - 1; // Move index to end of function name
                if (j >= len || expr[j] != '(') {
                    prsr::errors.push_back("Position " + std::to_string(i) + ": function '" + funcName + "' missing opening parenthesis");
                }
                else {
                    lastWasOperator = true; // Functions act as operators
                }
                continue;
            }

            // Check for variable errors
            if (funcName.size() > 1 || std::isdigit(expr[j])) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": invalid variable name '" + funcName + "'. Variables must be a single letter without numbers.");
                i = j - 1; // Skip remaining letters
                continue;
            }
        }

        // Detect single - letter variable
        if (std::isalpha(current)) {
            inNumber = false;
            lastWasOperator = false;
            lastWasDecimal = false;
            lastWasOpeningParenthesis = false;

            if (i > 0 && std::isdigit(expr[i - 1])) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": variable cannot start with a number");
            }
            continue;
        }
    

        // Check for operator errors
        if (current == '+' || current == '-' || current == '*' || current == '/') {
            // If it's a negative sign and it's the first character (valid negative number)
            if (i == 0 && current == '-') {
                lastWasOperator = true;
                continue;
            }

            // Handle consecutive operators (e.g., --)
            if (current == '-' && lastWasNegativeSign) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": consecutive negative signs '--' are not allowed");
                continue;
            }

            // Handle cases where an operator (other than '-') appears right after '('
            if (lastWasOpeningParenthesis && current != '-') {
                prsr::errors.push_back("Position " + std::to_string(i) + ": invalid operator '" + std::string(1, current) + "' after opening parenthesis");
            }

            // If it's a negative sign (-) after an opening parenthesis, allow it
            if (current == '-' && lastWasOpeningParenthesis) {
                lastWasOperator = false;
                lastWasNegativeSign = true; // Mark that we've encountered a negative sign
                continue;
            }

            // If there's an operator right after another operator, it's an error
            if (lastWasOperator) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": operator '" + std::string(1, current) + "' after another operator");
            }

            lastWasOperator = true;
            lastWasDecimal = false;
            inNumber = false;
            lastWasOpeningParenthesis = false; // Reset after operator
            continue;
        }

        // Check for decimal point errors
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
            lastWasOpeningParenthesis = false;
            continue;
        }

        // If the character is a number
        if (std::isdigit(current)) {
            inNumber = true;
            lastWasOperator = false;
            lastWasOpeningParenthesis = false;
            continue;
        }


        // Check for parentheses errors
        if (current == '(') {
            parenthesesCount++;
            lastWasOpeningParenthesis = true;
            lastWasOperator = false;
            lastWasDecimal = false;
            inNumber = false;
            lastWasNegativeSign = false; // Reset negative sign status
            continue;
        }
        else if (current == ')') {
            // Check if we have an operator or an empty parenthesis just before the closing parenthesis
            if (lastWasOpeningParenthesis) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": empty parentheses detected");
            }
            else if (lastWasOperator) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": closing parenthesis after an operator");
            }

            // Check if there are extra closing parentheses
            if (parenthesesCount == 0) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": extra closing parenthesis");
            }
            else {
                parenthesesCount--;
            }

            // Reset flags after a closing parenthesis
            lastWasOperator = false;
            lastWasDecimal = false;
            lastWasOpeningParenthesis = false;
            lastWasNegativeSign = false;
            inNumber = false;
            continue;
        }
    }

    // Check for unbalanced parentheses
    if (parenthesesCount > 0) {
        prsr::errors.push_back("Missing closing parentheses: " + std::to_string(parenthesesCount));
    }

    // Check if the expression ends with an operator
    if (lastWasOperator && len > 0) {
        prsr::errors.push_back("Position " + std::to_string(len - 1) + ": end of expression after an operator, expected a variable or number");
    }

    return prsr::errors;
}



void prsr::displayErrors(const std::vector<std::string>& errors) {
    if (errors.empty()) {
        ImGui::Text("The expression is correct");
    }
    else {
        ImGui::Text("Errors found in the expression:");
        for (const auto& error : errors) {
            ImGui::Text("%s", error.c_str());
        }
    }
}
