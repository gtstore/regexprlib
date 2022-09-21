#include <iostream>
#include "../regexprlib/regexpr.h"

int main(int argc, char *argv[]) {
	//RegExpr re{ "ab|xy|wz" };
	//std::string str{ "cccabvvvxy" };
	RegExpr re{ "(a|b)*c*" };
    //RegExpr re{ "c*" };
	std::string str{ "baaaaaaaaaaababaaabcccccxycccccc" };
    //std::string str{ "cccddd" };

	auto match1 = re.match(str);
    auto match0 = re.full_match(str);
		
	std::cout << "String: " + str << std::endl;

    std::cout << "Match: ";
    if (match1) std::cout << "'" << *match1 << "'" << std::endl;
    else std::cout << "Your regular expression does not match the subject string." << std::endl;
    std::cout << std::endl;

	std::cout << "Full match:" << std::endl;
	for(int i = 1; auto& v : match0) {
        std::cout << "Match" << std::to_string(i) << ": " << "'" << v << "'" << std::endl;
        ++i;
    }
	if (match0.empty()) std::cout << "Your regular expression does not match the subject string." << std::endl;

    return 0;
}

