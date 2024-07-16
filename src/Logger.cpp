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


Logger::Logger() : debug("DEBUG"), info("INFO"), warning("WARNING"), error("ERROR"),
                    debug_(NULL), info_(NULL), warning_(NULL), error_(NULL) {
}

Logger::Logger (const std::string &debug_file, const std::string &info_file,
                const std::string &warning_file, const std::string &error_file)
  : debug("DEBUG"), info("INFO"), warning("WARNING"), error("ERROR"),
    debug_(NULL), info_(NULL), warning_(NULL), error_(NULL) {
  debug_ = get_file_log_stream("DEBUG", debug_file);
  info_ = get_file_log_stream("INFO", info_file);
  warning_ = get_file_log_stream("WARNING", warning_file);
  error_ = get_file_log_stream("ERROR", error_file);
  if (debug_) {
    debug = *debug_;
  }

}

Logger::~Logger() {
  if (debug_) {
    delete debug_;
  }
  if (info_) {
    delete info_;
  }
  if (warning_) {
    delete warning_;
  }
  if (error_) {
    delete error_;
  }
}

LoggerStream *Logger::get_file_log_stream(const std::string &prefix, const std::string &filename) {
  std::ofstream *file_stream = new std::ofstream;
  file_stream->open(filename);
  if (!file_stream->is_open()) {
    std::cerr << "Could not open log file " << filename << std::endl;
    delete file_stream;
    return NULL;
  }
  LoggerStream *logger_stream = new LoggerStream(prefix, &std::clog, file_stream);
  return logger_stream;
}


void Logger::log_warning(const std::string &log_lvl, const std::string &message) const {
  std::cout << "[" << log_lvl << "] " << message << std::endl;
}

void Logger::log_warning(const std::string &message) const {
  std::cout << "[WARNING] " << message << std::endl;
}

void Logger::log_error(const std::string &message) const {
  std::cout << "[ERROR] " << message << std::endl;
}

void Logger::log_info(const std::string &message) const {
  std::cout << "[INFO] " << message << std::endl;
}

void Logger::log_debug(const std::string &message) const {
  std::cout << "[DEBUG] " << message << std::endl;
}
