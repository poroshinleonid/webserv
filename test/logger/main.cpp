#include <string>
#include <iostream>
#include <fstream>

#include "../../src/Logger.hpp"

int main() {
  Logger logger;

  std::string s = "HELLO BRUH\n";
  logger.debug << s;
  logger.warning << s;
  logger.info << s;
  logger.error << s;
}