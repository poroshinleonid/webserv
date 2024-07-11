#include "ServerConfig.hpp"

ServerConfig::ServerConfig() {}

ServerConfig::ServerConfig(ServerConfig const &other) : {};

ServerConfig::~ServerConfig() {}

ServerConfig	&ServerConfig::operator=(const ServerConfig &other) {
  if (this != &other) {
  }
  return (*this);
}
