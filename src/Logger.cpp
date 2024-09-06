#include "Logger.hpp"
#include <iostream>
#include <string>

Logger::Logger() : out_file_stream_(NULL) {}

Logger::Logger(const std::string &log_file) {
  std::ofstream *out_stream = new std::ofstream;
  out_stream->open(log_file.c_str(), std::ios_base::app);
  if (out_stream->is_open()) {
    out_file_stream_ = out_stream;
    return;
  }
  out_file_stream_ = NULL;
  delete out_stream;
  return;
}

Logger::~Logger() {
  if (out_file_stream_ != NULL) {
    delete out_file_stream_;
  }
}

void Logger::log(const std::string &log_lvl, const std::string &message) const {
  std::cout << "[" << log_lvl << "] " << message << std::endl;
  if (out_file_stream_ != NULL) {
    *out_file_stream_ << "[" << log_lvl << "] " << message << std::endl;
  }
}

void Logger::log_warning(const std::string &message) const {
  log("WARNING", message);
}

void Logger::log_error(const std::string &message) const {
  log("ERROR", message);
}

void Logger::log_info(const std::string &message) const {
  log("INFO", message);
}

void Logger::log_debug(const std::string &message) const {
  log("DEBUG", message);
}
