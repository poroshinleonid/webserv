#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <ostream>

enum Code {
    FG_RED      = 31,
    FG_GREEN    = 32,
    FG_BLUE     = 34,
    FG_MAGENTA  = 35,
    FG_YELLOW   = 33,
    FG_DEFAULT  = 39,
};
struct Modifier {
    Code code;
    Modifier(Code pCode) : code(pCode) {}
    friend std::ostream&
    operator<<(std::ostream& os, const Modifier& mod) {
        return os << "\033[" << mod.code << "m";
    }
};


class Logger {
private:
  Logger &operator=(const Logger &obj);
  Logger(Logger const &other);

public:
  Logger();
  Logger(const std::string &log_file);
  ~Logger();

public:
  void log(const std::string &log_lvl, const std::string &message, Modifier log_lvl_color, Modifier msg_color) const;
  void log_debug(const std::string &message) const;
  void log_info(const std::string &message) const;
  void log_warning(const std::string &message) const;
  void log_error(const std::string &message) const;

private:
  std::ofstream *out_file_stream_;
};

