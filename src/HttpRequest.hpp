#ifndef HttpRequest_HPP
#define HttpRequest_HPP

#include <string>

class	HttpRequest {
public:
  HttpRequest();
  HttpRequest(HttpRequest const &other);
  ~HttpRequest();
  HttpRequest &operator=(const HttpRequest &obj);

public:
  HttpRequest(const std::string &request_string);

public:
  bool is_valid;
  bool is_for_cgi;
private:
};

#endif
