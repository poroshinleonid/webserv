#include <iostream>
#include "../../src/Config.hpp"

int main() {
  Config config("../parsing/e.cfg");
  std::cout << config["server"].get_content() << std::endl;
  std::cout << config["server"]["lastkey"].get_content() << std::endl;
  std::cout << config["server"]["lastkey"].unwrap() << std::endl;
  std::cout << config["server"]["firstkey"].unwrap() << std::endl;
  // std::cout << config["server"]["sus"].unwrap() << std::endl;
}