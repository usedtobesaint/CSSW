#include "parser.h"
#include <stack>
#include <string>
#include <iostream>
#include "../imgui/imgui.h"


using namespace prsr;

Node* createNode(const std::string& value) {
    return new Node{ value, {} };
}

// A simple parser to create a tree from the expression
prsr::Node* prsr::parseExpression(const std::string& expr) {
    Node* root = nullptr;
    Node* currentOperatorNode = nullptr;

    for (char ch : expr) {
        if (ch == '+' || ch == '-' || ch == '*' || ch == '/') {
            // Create a new operator node
            Node* operatorNode = createNode(std::string(1, ch));

            if (!root) {
                // If root is null, set this operator as root
                operatorNode->children.push_back(currentOperatorNode);
                root = operatorNode;
            }
            else {
                // Attach the current tree to the left of the new operator
                operatorNode->children.push_back(root);
                root = operatorNode;
            }
            currentOperatorNode = operatorNode;
        }
        else {
            // Create operand node
            Node* operandNode = createNode(std::string(1, ch));

            if (!root) {
                root = operandNode;
            }
            else if (currentOperatorNode) {
                // Attach operand to the operator's right
                currentOperatorNode->children.push_back(operandNode);
                currentOperatorNode = nullptr; // Reset to allow a new operator to take precedence
            }
        }
    }
    return root;
}



// Function to draw the node in ImGui
void prsr::drawNode(Node* node) {
    if (node == nullptr) return;

    // Draw the node value
    ImGui::Text("%s", node->value.c_str());
    std::cout << "Node: " << node->value << std::endl; // Debug output

    // If there are children, draw them
    if (!node->children.empty()) {
        ImGui::Indent(); // Indent for child nodes
        for (Node* child : node->children) {
            std::cout << "Child of " << node->value << ": " << child->value << std::endl; // Debug output
            drawNode(child); // Recursively draw each child
        }
        ImGui::Unindent(); // Unindent after drawing children
    }
}

