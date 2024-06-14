#include "Config.hpp"

#include <vector>
#include <cctype>

using std::vector;
using std::string;

string remove_spaces(const string& s) {
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

/*
def remove_spaces(s):
    i = 0
    res = []
    while i < len(s):
        if (s[i] == '"'):
            res.append('"')
            i += 1
            while i < len(s) and s[i] != '"':
                res.append(s[i])
                i += 1
        if i < len(s) and not s[i].isspace():
            res.append(s[i])
        i += 1
    return ''.join(res)
*/