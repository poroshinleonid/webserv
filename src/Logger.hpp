#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <vector>
#include <fstream>
#include "LoggerStream.hpp"

class	Logger {
private:
  Logger &operator=(const Logger &obj);
  Logger(Logger const &other);

public:
  Logger();
  Logger (const std::string &debug_file, const std::string &info_file,
                const std::string &warning_file, const std::string &error_file);
  ~Logger();

private:
  LoggerStream *get_file_log_stream(const std::string &prefix, const std::string &filename);

  // void log(const std::string message) const;

public:
  LoggerStream *debug;
  LoggerStream *info;
  LoggerStream *warning;
  LoggerStream *error;
  std::vector<std::ofstream> file_streams;
};

#endif // LOGGER_HPP
