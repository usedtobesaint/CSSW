#pragma once
#include <string>
#include <vector>
namespace prsr {

	inline char expression[256] = "";
	inline std::vector<std::string> errors;


	std::vector<std::string> checkExpression(char* expr); 
	void displayErrors(const std::vector<std::string>& errors);

}