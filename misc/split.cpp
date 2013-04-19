/**
 * This class can be used to tokenize a string with a delimiter.
 * @Author: J.Huber, IRT GmbH
 * Creation date: February 28, 2012
 */

#include "split.h"
#include <vdr/tools.h>

cSplit::cSplit () : tokens (0) {
}

cSplit::~cSplit(){
	tokens.clear();
	delete &tokens;
}

void cSplit::split(const std::string& myStr, const std::string& delimiter){
	std::string s;
	for(std::size_t i = 0; i < myStr.size(); i++){
		if( (myStr.substr(i, delimiter.size()).compare(delimiter) == 0)){
			tokens.push_back(s);
			s.clear();
			i += delimiter.size() - 1;
		}else{
			s.append(1, myStr[i]);
			if(i == (myStr.size() - 1))
			tokens.push_back(s);
		}
	}
}

std::vector<std::string>* cSplit::split (const char* text, const char* delimiter){
	tokens.clear();
	if (text && *text){
		const std::string m1 (text), m2 (delimiter);
		split(m1, m2);
	}
	return &tokens;
}