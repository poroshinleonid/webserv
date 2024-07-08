#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>

class	Logger {
public:
  Logger();
  Logger(Logger const &other);
  ~Logger();
  Logger &operator=(const Logger &obj);
  void log(const std::string message) const;

  // operator<<
private:
};

#endif // LOGGER_HPP
