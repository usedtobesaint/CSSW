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

// List of available functions
const std::vector<std::string> functions = { "SIN", "COS", "TAN", "SQRT" };

std::vector<std::string> prsr::checkExpression(const char* expr) {
    int parenthesesCount = 0; // Counter for open parentheses
    bool lastWasOperator = false; // Start as true to allow a variable first
    bool lastWasDecimal = false; // Track if the last number had a decimal point
    bool inNumber = false; // Track if we're inside a number
    bool lastWasOpeningParenthesis = false; // Track if the last character was an opening parenthesis
    bool lastWasNegativeSign = false; // Track if the last character was a negative sign

    std::cout << "Current expression: " << expr << std::endl;
    size_t len = strlen(expr);

    // Check if the first character is an invalid operator
    if (len > 0 && (expr[0] == '+' || expr[0] == '*' || expr[0] == '/')) {
        prsr::errors.push_back("Position 0: expression starts with an invalid operator '" + std::string(1, expr[0]) + "'");
    }

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
                    lastWasOperator = false;
                }
                continue;
            }
        }

        // Detect single - letter variable
        if (std::isalpha(current)) {
            std::string varName;
            int j = i;
            // Collect all letters to form potential variable name
            while (j < len && std::isalpha(expr[j])) {
                varName += expr[j];
                ++j;
            }

            // Check if variable name is invalid
            if (varName.size() > 1 || (j < len && std::isdigit(expr[j]))) {
                prsr::errors.push_back("Position " + std::to_string(i) + ": invalid variable name '" + varName + "'");
                i = j - 1; // Skip over invalid characters
                continue;
            }
            lastWasOperator = false;
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
        prsr::errors.push_back("Missing closing parenthesis: " + std::to_string(parenthesesCount));
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


std::string prsr::correctExpression(char* expr, const std::vector<std::string>& errors) {
    std::string result;
    std::string currentExpr=expr;
    int parenthesis = 0;
    bool isModified = false;
    bool hasError = true;

    while (hasError) {
        std::cout << "Current expression lenght:" << strlen(currentExpr.c_str()) << std::endl;

        for (const auto& error : errors) {
            std::cout << "Error: " << error << std::endl;
        }
        // Check for missing closing parenthesis in errors and parse the count if present
        for (const auto& error : errors) {
            if (error.find("Missing closing parenthesis:") != std::string::npos) {
                // Extract the number of missing parentheses using regex
                std::regex numRegex(R"((\d+))"); // Matches any number in the error message
                std::smatch match;
                if (std::regex_search(error, match, numRegex)) {
                    parenthesis = std::stoi(match.str(0)); // Set parenthesis count to parsed value
                }
            }
        }

        // Process each character in expr and apply corrections based on error positions
        for (int i = 0; i < strlen(currentExpr.c_str()); ++i) {
            for (const auto& error : errors) {
                // Handle errors with positions only
                size_t posStart = error.find("Position ");
                if (posStart != std::string::npos) {
                    size_t posEnd = error.find(':', posStart + 9);
                    int errorPos = std::stoi(error.substr(posStart + 9, posEnd - (posStart + 9)));

                    if (errorPos == i) {
                        if (error.find("invalid character") != std::string::npos) {
                            isModified = true;
                        }
                        else if (error.find("starts with an invalid") != std::string::npos) {
                            result += '0';
                        }
                        else if (error.find("invalid variable name") != std::string::npos) {
                            // Skip over the invalid variable name
                            while (i < strlen(currentExpr.c_str()) && (std::isalpha(currentExpr[i]) || std::isdigit(currentExpr[i]))) {
                                ++i;
                            }
                            --i;
                            isModified = true;
                        }
                        else if (error.find("consecutive negative signs") != std::string::npos ||
                            error.find("after opening parenthesis") != std::string::npos ||
                            error.find("after another operator") != std::string::npos ||
                            error.find("second decimal point in the number") != std::string::npos ||
                            error.find("extra closing parenthesis") != std::string::npos) {
                            isModified = true; // Mark for removal
                        }
                        else if (error.find("decimal point without a number") != std::string::npos ||
                            error.find("after an operator") != std::string::npos) {
                            result += '0'; // Add '0' before the decimal or for missing variable
                        }
                        else if (error.find("empty parentheses detected") != std::string::npos) {
                            result += "0"; // Replace empty parentheses with '(0)'
                            isModified = true;
                        }
                        else if (error.find("end of expression after an operator") != std::string::npos) {
                            result += currentExpr[i];
                            result += '0'; // Add '0' to end the expression
                            isModified = true;
                        }
                    }
                }
            }

            // If not modified, add the current character to the result
            if (!isModified) {
                result += currentExpr[i];
            }

            isModified = false;  // Reset modification flag after each character

            std::cout << "Current expression: " << result << " at position: " << i << std::endl;
        }

        std::cout << parenthesis << std::endl;

        // Append missing closing parentheses as determined by parsed value
        for (int k = 0; k < parenthesis; k++) {
            result += ')';
        }

        prsr::errors.clear();
        prsr::errors = prsr::checkExpression(result.c_str());

		if (errors.empty()) {
			hasError = false;
		}
        currentExpr = result;
        result = "";
    }
    return currentExpr; // Return the modified expression
}
