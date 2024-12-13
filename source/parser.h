#pragma once
#include <string>
#include <vector>
namespace prsr {

	struct Node {
		std::string value;
		std::vector<Node*> children;
	};

	inline char expression[256] = "";
	inline std::string correctedExpression = "";
	inline std::vector<std::string> errors;
	inline Node* tree = nullptr;
	//syntax errors
	std::vector<std::string> checkExpression(const char* expr);
	//errors
	void displayErrors(const std::vector<std::string>& errors);
	//correcting expression
	std::string correctExpression(char* expr, const std::vector<std::string>& errors);

	Node* parseExpression(const std::string& expr);
	void drawNode(Node* node);
}

