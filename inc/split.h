/*
 *  File:   split.h
 *  Author: J.Huber, IRT GmbH
 *  Date:   February 28, 2012
 *  Last modification: February 18, 2013
 */
#ifndef __SPLIT_H
#define __SPLIT_H

#include <iostream>
#include <vector>
#include <string>

/**
 * This helper class can be used to tokenize a text string using a delimiter.
 * The result is returned as a vector with string elements.
 */

class cSplit {
private:
	std::vector<std::string> tokens;
    void split(const std::string&, const std::string&);

public:
  /**
   * Construct an instance of the class.
   */
   cSplit ();
   ~cSplit();
   /**
    * Tokenize a string using a delimiter.
    * @param text the string to be tokenized
    * @param delimiter the delimiter string used
    * @return a pointer to the <code>vector</code> with the resulting string tokens
    */
   std::vector<std::string>* split(const char* text, const char* delimiter);
};

#endif //__SPLIT_H