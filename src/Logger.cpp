#include "Logger.hpp"
#include <string>
#include <iostream>
#include "LoggerStream.hpp"

// deleted
// Logger::Logger(Logger const &other) : {};
// Logger	&Logger::operator=(const Logger &other) {
//   if (this != &other) {
//   }
//   return (*this);
// }


Logger::Logger() : debug(NULL), info(NULL), warning(NULL), error(NULL) {
  debug = new LoggerStream("DEBUG");
  info = new LoggerStream("INFO");
  warning = new LoggerStream("WARNING");
  error = new LoggerStream("ERROR");
}

Logger::Logger (const std::string &debug_file, const std::string &info_file,
                const std::string &warning_file, const std::string &error_file) : Logger() {
  debug = get_file_log_stream("DEBUG", debug_file);
  info = get_file_log_stream("INFO", info_file);
  warning = get_file_log_stream("WARNING", warning_file);
  error = get_file_log_stream("ERROR", error_file);
}

Logger::~Logger() {
  if (debug) {
    delete debug;
  }
  if (info) {
    delete info;
  }
  if (warning) {
    delete warning;
  }
  if (error) {
    delete error;
  }
}

LoggerStream *Logger::get_file_log_stream(const std::string &prefix, const std::string &filename) {
  try {
    std::ofstream *file_stream = new std::ofstream;
    file_stream->open(filename);
    if (!file_stream->is_open()) {
      std::cerr << "Could not open log file " << filename << std::endl;
      delete file_stream;
      return NULL;
    }
    LoggerStream *logger_stream = new LoggerStream(prefix, &std::clog, file_stream);
    return logger_stream;
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return NULL;
  }
}




// void Logger::log(const std::string message) const {
//   std::cout << message << std::endl;
// }
