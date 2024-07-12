#ifndef LoggerStream_HPP
#define LoggerStream_HPP


#include <string>
#include <vector>
#include <fstream>
#include <iostream>

class	LoggerStream : std::ostream {
// deleted constructors
private:
  LoggerStream(LoggerStream const &other);
public:
  LoggerStream();
  LoggerStream(const std::string &prefix);
  LoggerStream(const std::string &prefix, std::ostream *out_stream, std::ofstream *out_file);
  ~LoggerStream();
  LoggerStream &operator=(LoggerStream &obj);
public:
  template <typename T>
  LoggerStream& operator<<(const T& value) {
      if (out_stream_ != NULL) {
        *out_stream_ << "[" << log_prefix << "] " << value;
      }
      if (out_file_ != NULL) {
        *out_file_ << "[" << log_prefix << "] " << value;
      }
     return *this;
  }

// FIX: add log level prefix
// FIX: add the ability to write to both stream and file at the same time (needs a new constructor)
// loggerstream(std::ostream *out_stream, std::ofstream *out_file) for a universal constructor
private:
  std::string log_prefix;
  std::ostream *out_stream_;
  std::ofstream *out_file_;
};

#endif
