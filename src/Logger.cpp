#include "Logger.hpp"
#include <iostream>
#include <string>

Modifier Creset(FG_DEFAULT);
Modifier Cdefault(FG_DEFAULT);
Modifier Cred(FG_RED);
Modifier Cgreen(FG_GREEN);
Modifier Cblue(FG_BLUE);
Modifier Cpink(FG_MAGENTA);
Modifier Cyellow(FG_YELLOW);

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

void Logger::log(const std::string &log_lvl, const std::string &message, Modifier log_lvl_color=Cdefault, Modifier msg_color=Cdefault) const {
  std::cout << log_lvl_color << "[" << log_lvl << "] " << Creset << msg_color << message << Creset << std::endl;
  if (out_file_stream_ != NULL) {
    *out_file_stream_ << "[" << log_lvl << "] " << message << std::endl;
  }
}

void Logger::log_warning(const std::string &message) const {
  log("WARNING", message, Cyellow, Cyellow);
}

void Logger::log_error(const std::string &message) const {
  log("ERROR", message, Cred, Cred);
}

void Logger::log_info(const std::string &message) const {
  log("INFO", message, Cgreen, Cdefault);
}

void Logger::log_debug(const std::string &message) const {
  (void)message;
  #ifdef DEBUG
    log("DEBUG", message, Cpink, Cdefault);
  #endif
}
