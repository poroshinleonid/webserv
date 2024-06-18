#include "Config.hpp"

#include <vector>
#include <cctype>

using std::vector;
using std::string;

// '  {  "a b"   : "c d"  } ' -> '{"a b":"c d"}'
string Config::remove_spaces(const string& s) {
    string res = "";
    bool in_quote = false;
    for (std::string::const_iterator it = s.begin(); it < s.end(); it++) {
        if (*it == '"')
            in_quote = !in_quote;
        if (in_quote || !isspace(static_cast<unsigned char>(*it)))
            res += *it;
    }
    return res;
}

