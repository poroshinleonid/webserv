#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <iostream>
#include <future>
#include <chrono>

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

// returns true if first is prefix is prefix of vec
template <typename T>
bool is_vector_prefix(const std::vector<T>& prefix, const std::vector<T>& vec) {
    if (prefix.size() > vec.size()) {
        return false;
    }
    for (size_t i = 0; i < prefix.size(); i++) {
        if (prefix[i] != vec[i]) {
            return false;
        }
    }
    return true;
}


// parody on rust's dbg! macro
template <typename T>
T dbg(T thingy) {
    std::cerr << "debug: " << thingy << '\n';
    return thingy;
}

// checks if future is ready
template<typename R>
bool future_is_ready(std::future<R> const& f) { 
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; 
}
