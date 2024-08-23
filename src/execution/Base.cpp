#include "Base.hpp"

#include <algorithm> 
#include <cctype>
#include <locale>

std::string ltrim(const std::string &s) {
    std::string s_cp = s;
    s_cp.erase(s.begin(), std::find_if(s_cp.begin(), s_cp.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

std::string rtrim(const std::string &s) {
    std::string s_cp = s;
    s_cp.erase(std::find_if(s_cp.rbegin(), s_cp.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s_cp.end());
    return s_cp;
}

std::string trim(const std::string &s) {
    return ltrim(rtrim(s));
}

std::istream& getline_str(std::istream& istream, std::string& s, const std::string& sep) {
    s = "";
    char c;
    while (istream.get(c)) {
        s += c;
        if (s.find(sep) != std::string::npos) {
            s = s.substr(0, s.find(sep));
            break;
        }
    }
    return istream;
}
