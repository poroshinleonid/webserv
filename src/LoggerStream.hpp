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
  LoggerStream &operator=(const LoggerStream &obj);
public:
  LoggerStream();
  LoggerStream(const std::string &prefix);
  LoggerStream(const std::string &prefix, std::ostream *out_stream, std::ofstream *out_file);
  ~LoggerStream();
public:
  template <typename T>
  LoggerStream& operator<<(const T& value) {
      if (out_file_) {
        *out_stream_ << value;
      }
      if (out_file_) {
        *out_file_ << value;
      }
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
