#include "HttpRequest.hpp"

HttpRequest::HttpRequest() {}

HttpRequest::HttpRequest(HttpRequest const &other) : {};

HttpRequest::~HttpRequest() {}

HttpRequest	&HttpRequest::operator=(const HttpRequest &other) {
  if (this != &other) {
  }
  return (*this);
}
