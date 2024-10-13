#pragma once

#include <string>
#include <sstream>

// trim from start
std::string ltrim(const std::string &s);

// trim from end
std::string rtrim(const std::string &s);

// trim both start and end
std::string trim(const std::string &s);

// getline but with string separator
std::istream& getline_str(std::istream& istream, std::string& s, const std::string& sep);
