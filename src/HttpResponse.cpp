#include "HttpResponse.hpp"

HttpResponse::HttpResponse() {}

HttpResponse::HttpResponse(HttpResponse const &other) : {};

HttpResponse::~HttpResponse() {}

HttpResponse	&HttpResponse::operator=(const HttpResponse &other) {
  if (this != &other) {
  }
  return (*this);
}
