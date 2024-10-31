#include "Libft.hpp"
#include "Config.hpp"

#include <cstdlib>
#include <sstream>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>

Libft::Libft() {}

Libft::Libft(Libft const &other) { (void)other; };

Libft::~Libft() {}

Libft &Libft::operator=(const Libft &other) {
  (void)other;
  return (*this);
}

int Libft::ft_atoi(const std::string &s) {
  return static_cast<int>(std::strtod(s.c_str(), NULL));
}

 char Libft::tolower(int c) {
  if (c >= 'A' && c <= 'Z') {
    c -= 'A';
    c += 'a';
  }
  return c;
 }

 char Libft::toupper(int c) {
  if (c >= 'a' && c <= 'z') {
    c -= 'a';
    c += 'A';
  }
  return c;
 }

 char Libft::toenv(int c) {
  c = toupper(c);
  if (c == ' ' || c == '-') {
    c = '_';
  }
  return c;
 }

std::string Libft::ft_itos(int number) {
  std::ostringstream strm;
  std::string s;
  strm << number;
  s = strm.str();
  return s;
}

void Libft::ft_memset(void *dst, int len, unsigned char byte) {
  unsigned char *p = (unsigned char *)dst;
  for (int i = 0; i < len; i++) {
    p[i] = byte;
  }
}
