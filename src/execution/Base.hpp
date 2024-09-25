#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <array>

// trim from start
std::string ltrim(const std::string &s);

// trim from end
std::string rtrim(const std::string &s);

// trim both start and end
std::string trim(const std::string &s);

// getline but with string separator
std::istream& getline_str(std::istream& istream, std::string& s, const std::string& sep);

// splits line by delimiter
std::vector<std::string> split(const std::string& str, char del);

// splits only one time (or doesn't split if delimiter is not found)
std::vector<std::string> split_one(const std::string& str, char del);
