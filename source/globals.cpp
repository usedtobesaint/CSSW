#include "parser.h"

namespace prsr {
    // Define the global variables declared as extern in parser.h
    std::vector<std::string> errors;
    char expression[256] = "";
    std::string correctedExpression;
    std::string optimizedExpression;
}