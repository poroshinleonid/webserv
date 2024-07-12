#ifndef HttpResponse_HPP
#define HttpResponse_HPP

#include "HttpRequest.hpp"

#include <string>
#include <map>

class	HttpResponse {
public:
  HttpResponse();
  HttpResponse(HttpResponse const &other);
  ~HttpResponse();
  HttpResponse &operator=(const HttpResponse &obj);

public:
  std::string to_string() const;

// internal workings data
public:
  bool is_valid;
  bool is_for_cgi;
  std::string client_hostname;
  std::string client_port;

// Actual request fields
public:
  int status_code;
  std::string status_message;
  std::string header;
  std::string body;
  http_method method;
  std::string uri;
  std::string version;
  std::map<std::string, std::string> header_params;

};

#endif
