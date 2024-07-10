#include "HttpConnection.hpp"

HttpConnection::HttpConnection() {}

HttpConnection::HttpConnection(HttpConnection const &other) : {
  (void)other;
};

HttpConnection::~HttpConnection() {}

HttpConnection	&HttpConnection::operator=(const HttpConnection &other) {
  (void)other;
  if (this != &other) {
  }
  return (*this);
}
