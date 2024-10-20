#include <fstream>
#include <iostream>
#include <sstream>
#include <cassert>
#include "../../src/Config.hpp"

using std::string;

// check empty keys
// check dublicate keys
// check empty values

void CheckIfInvalid(std::string path) {
    try {
        Config cfg(path);
        std::cout << path << " invalid test failed\n";
        throw std::exception();
    } catch (InvalidConfig) {
        std::cout << path << " is indeed invalid\n";
    }
}

void assert_vec_config_eq(std::vector<Config> first, std::vector<std::string> second) {
    assert(first.size() == second.size());
    for (int i = 0; i < first.size(); i++) {
        assert(first[i].get_content() == second[i]);
    }
}


int main() {
    Config a("ConfigNormal.cfg");
    assert(a["server"]["routes"]["login"].unwrap() == "tae");
    assert(a["api"]["timeout"].unwrap() == "30");

    try {
        a["server"].unwrap();
        throw std::exception();
    }
    catch (std::invalid_argument) {}

    try {
        a["not found"];
        throw std::exception();
    }
    catch (std::out_of_range) {}

    try {
        a.unwrap();
        throw std::exception();
    }
    catch (std::invalid_argument) {}

    std::cout << a["empty_value"].unwrap();
    std::cout << "Basic tests passed\n";
    CheckIfInvalid("Empty.cfg");
    CheckIfInvalid("Random.cfg");
    CheckIfInvalid("NonClosing.cfg");
    CheckIfInvalid("NonExisting.cfg");
    CheckIfInvalid("EmptyKey.cfg");
    Config whatever("DublicateValue.cfg"); 
    Config whatever2("DublicateValueDeep.cfg");
    assert(whatever2["a"]["b"]["c"].unwrap() == "g");
    std::vector<string> expected;
    expected.push_back("b");
    expected.push_back("g");
    assert_vec_config_eq(whatever.get_vec("a"), expected);
    assert_vec_config_eq(whatever2["a"]["b"].get_vec("c"), expected);
    Config def;
    std::cout << "default constructor works\n";
}
