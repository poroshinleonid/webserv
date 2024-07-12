#include "LoggerStream.hpp"

#include <iostream>



LoggerStream::LoggerStream() :log_prefix("[LOG]"), out_stream_(&std::cout), out_file_(NULL) {}

LoggerStream::LoggerStream(const std::string &prefix)
  : log_prefix(prefix), out_stream_(&std::clog), out_file_(NULL) {}

LoggerStream::LoggerStream(const std::string &prefix, std::ostream *out_stream, std::ofstream *out_file)
  : log_prefix(prefix), out_stream_(NULL), out_file_(NULL) {
  if (out_stream) {
    out_stream_ = out_stream;
  }
  if (out_file) {
    out_file_ = out_file;
  }
}

LoggerStream::~LoggerStream() {
  if (out_file_ != NULL) {
    out_file_->close();
    delete out_file_;
  }
}

LoggerStream &LoggerStream::operator=(LoggerStream &obj) {
  log_prefix = obj.log_prefix;
  out_stream_ = obj.out_stream_;
  out_file_ = obj.out_file_;
  obj.out_file_ = NULL;
  return *this;
}

