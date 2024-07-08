#include "Logger.hpp"
#include <string>
#include <iostream>

Logger::Logger() {}

Logger::Logger(Logger const &other) : {};

Logger::~Logger() {}

Logger	&Logger::operator=(const Logger &other) {
  if (this != &other) {
  }
  return (*this);
}

void Logger::log(const std::string message) const {
  std::cout << message << std::endl;
}
