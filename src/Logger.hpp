#pragma once

#include <fstream>
#include <string>
#include <vector>

class Logger {
private:
  Logger &operator=(const Logger &obj);
  Logger(Logger const &other);

public:
  Logger();
  Logger(const std::string &log_file);
  ~Logger();

public:
  void log(const std::string &log_lvl, const std::string &message) const;
  void log_debug(const std::string &message) const;
  void log_info(const std::string &message) const;
  void log_warning(const std::string &message) const;
  void log_error(const std::string &message) const;

private:
  std::ofstream *out_file_stream_;
};

