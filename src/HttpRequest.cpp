#include "HttpRequest.hpp"

#include <string>

HttpRequest::HttpRequest() {}

HttpRequest::HttpRequest(HttpRequest const &other) : {};

HttpRequest::~HttpRequest() {}

HttpRequest	&HttpRequest::operator=(const HttpRequest &other) {
  if (this != &other) {
  }
  return (*this);
}

HttpRequest::HttpRequest(const std::string &request_string) {
  // FIX probly parse the string here
  // and set a var isValid trve or false
}

