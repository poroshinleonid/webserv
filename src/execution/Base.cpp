#include "Base.hpp"

#include <algorithm> 
#include <cctype>
#include <locale>

std::string ltrim(const std::string &s) {
    std::string s_cp = s;
    s_cp.erase(s_cp.begin(), std::find_if(s_cp.begin(), s_cp.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    return s_cp;
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

std::vector<std::string> split(const std::string& str, char del) {
    std::vector<std::string> res;
    std::string s = str;
    while (!s.empty()) {
        size_t pos = s.find(del);
        if (pos == std::string::npos) {
            res.push_back(std::move(s));
        } else {
            res.push_back(s.substr(0, pos));
            s = s.substr(pos + 1);
        }
    }
    return res;
}

std::vector<std::string> split_one(const std::string& str, char del) {
    std::vector<std::string> res;
    size_t pos = str.find(del);
    if (pos == std::string::npos) {
        res.push_back(str);
        return res;
    }
    res.push_back(str.substr(0, pos));
    res.push_back(str.substr(pos + 1));
    return res;
}
