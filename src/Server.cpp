#include "Server.hpp"

#include <iostream>

Server::Server(): srv_sockaddr({0, 0, 0, 0}),
      client_max_body_size(0),
      port(0), host(""), timeout(0), cgi_timeout(0),
      listen_fd(0), is_default(false),
      server_name(""),
      client_body_size(0) {}

Server::~Server() {}

Server::Server(Server const &other)
    : srv_sockaddr(other.srv_sockaddr), server_names(other.server_names),
      error_pages(other.error_pages),
      client_max_body_size(other.client_max_body_size), routes(other.routes),
      port(other.port), host(other.host), timeout(other.timeout), cgi_timeout(other.cgi_timeout),
      listen_fd(other.listen_fd), is_default(other.is_default),
      server_name(other.server_name),
      default_error_pages(other.default_error_pages),
      client_body_size(other.client_body_size) {}

Server &Server::operator=(const Server &other) {
  if (this == &other) {
    return *this;
  }
  srv_sockaddr = other.srv_sockaddr;
  server_names = other.server_names; // - split by " "
  error_pages = other.error_pages;
  client_max_body_size = other.client_max_body_size;
  routes = other.routes;
  port = other.port;
  host = other.host;
  timeout = other.timeout;
  cgi_timeout = other.cgi_timeout;
  listen_fd = other.listen_fd;
  is_default = other.is_default;
  server_name = other.server_name;
  default_error_pages = other.default_error_pages;
  client_body_size = other.client_body_size;
  return *this;
}

#define DEBUG
#ifdef DEBUG
void Server::print_server() { std::cout << "*server*\n"; }
#endif
