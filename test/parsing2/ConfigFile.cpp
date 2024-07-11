#include "ConfigFile.hpp"

ConfigFile::ConfigFile() {}

ConfigFile::ConfigFile(ConfigFile const &other) : {};

ConfigFile::~ConfigFile() {}

ConfigFile	&ConfigFile::operator=(const ConfigFile &other) {
  if (this != &other) {
  }
  return (*this);
}
