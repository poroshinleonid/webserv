#include "Server.hpp"

Server::Server() {}

Server::Server(Server const &other) : {};

Server::~Server() {}

Server	&Server::operator=(const Server &other) {
  if (this != &other) {
  }
  return (*this);
}
