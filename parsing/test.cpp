#include <fstream>
#include <iostream>

int main() {
    std::fstream f;
    f.open("a.txt");
    std::cout << f.is_open();
    char a[10];
    f.read(a, 1);
}