#include "tree.h"
#include <stack>
#include <sstream>
#include <queue>
#include <iostream>
#include <regex>

using namespace tree;
tree::Node::Node(std::string val) : value(val) {}

std::vector<std::string> tree::tokenizeExpression(std::string expr) {
    std::vector<std::string> tokens;
    std::regex tokenRegex(R"(([a-zA-Z]+\()|([a-zA-Z_][a-zA-Z0-9_]*)|([0-9]+(?:\.[0-9]+)?)|([+\-*/()]))");
    std::sregex_iterator it(expr.begin(), expr.end(), tokenRegex);
    std::sregex_iterator end;

    while (it != end) {
        tokens.push_back(it->str());
        ++it;
    }
    return tokens;
}

Node* tree::buildParallelTree(const std::vector<std::string>& tokens) {
    std::stack<tree::Node*> nodeStack;

    for (const auto& token : tokens) {
        if (token == "+" || token == "-" || token == "*" || token == "/") {
            Node* right = nodeStack.top(); nodeStack.pop();
            Node* left = nodeStack.top(); nodeStack.pop();
            Node* opNode = new Node(token);
            opNode->children.push_back(left);
            opNode->children.push_back(right);
            nodeStack.push(opNode);
        }
        else {
            nodeStack.push(new Node(token));
        }
    }

    return nodeStack.top(); // Root of the tree
}

Node* tree::optimizeParallelTree(Node* root) {
    if (!root) return nullptr;

    // Base case: If it's a leaf node, return it
    if (root->children.empty()) return root;

    // Apply parallel optimization for associativity if possible
    if (root->value == "+" || root->value == "*") {
        std::queue<Node*> nodes;
        for (auto child : root->children) {
            if (child->value == root->value) {
                for (auto grandChild : child->children) {
                    nodes.push(grandChild);
                }
                delete child;
            }
            else {
                nodes.push(child);
            }
        }

        root->children.clear();
        while (!nodes.empty()) {
            root->children.push_back(nodes.front());
            nodes.pop();
        }
    }

    return root;
}

void tree::printTree(Node* root, int depth) {
    if (!root) return;

    for (int i = 0; i < depth; ++i) std::cout << "  ";
    std::cout << root->value << std::endl;

}


void tree::printTokens(const std::vector<std::string>& tokens) {
	for (const auto& token : tokens) {
		std::cout << token << " ";
	}
	std::cout << std::endl;
}