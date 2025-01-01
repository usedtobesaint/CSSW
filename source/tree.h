#ifndef PARALLELTREE_H
#define PARALLELTREE_H

#include <string>
#include <vector>
namespace tree {

    struct Node {
        std::string value;
        std::vector<Node*> children;

        Node(std::string val);
    };


    std::vector<std::string> tokenizeExpression(std::string expr);
    Node* buildParallelTree(const std::vector<std::string>& tokens);
    Node* optimizeParallelTree(Node* root);
    void printTree(Node* root, int depth = 0);
    void printTokens(const std::vector<std::string>& tokens);
}

#endif // PARALLELTREE_H