#include <fstream>
#include <iostream>

using std::string;

string remove_spaces(const string& s);

int main() {
    std::cout << remove_spaces("{\"a  \": \"  b  \"  }  ");
}