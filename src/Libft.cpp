#include "Libft.hpp"

#include <sstream>

#include <arpa/inet.h>

Libft::Libft() {}

Libft::Libft(Libft const &other) {
  (void)other;
};

Libft::~Libft() {}

Libft	&Libft::operator=(const Libft &other) {
  (void)other;
  return (*this);
}



int Libft::ft_atoi(const std::string &s) {
  return std::atoi(s.c_str());
}

int Libft::ft_atoip(const std::string &s) {
  int result;
  inet_pton(AF_INET, s.c_str(), &result); // FIX forbidden function
  return result;
}

std::string Libft::ft_itos(int number) {
  std::ostringstream strm;
  std::string s;
  strm << number;
  s = strm.str();
  return s;
}