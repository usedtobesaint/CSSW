#pragma once
#include <string>
#include <vector>
namespace prsr {

	inline char expression[256] = "";
	inline std::string correctedExpression = "";
	inline std::string optimizedExpression = "";
	inline std::vector<std::string> errors;
	//syntax errors
	std::vector<std::string> checkExpression(const char* expr);
	//errors
	void displayErrors(const std::vector<std::string>& errors);
	//correcting expression
	std::string correctExpression(char* expr, const std::vector<std::string>& errors);
	std::string optimizeExpression(std::string& expression);

}

