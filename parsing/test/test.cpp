#include <fstream>
#include <iostream>
#include <sstream>
#include <cassert>
#include "../Config.hpp"

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
    catch (std::invalid_argument) {}

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
    CheckIfInvalid("DublicateValue.cfg"); // TODO: make it pass
}
