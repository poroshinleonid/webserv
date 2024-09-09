#include "Libft.hpp"
#include "Config.hpp"

#include <sstream>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>


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

unsigned long Libft::ft_atoip(const std::string &s) {
  unsigned long result;
  // string_to_ip
  inet_pton(AF_INET, s.c_str(), &result); // LIBFT forbidden function
  return static_cast<unsigned long>(Config::string_to_ip(s));
}

std::string Libft::ft_iptoa(int ip_repr) {
  int a, b, c, d;
  std::ostringstream strm;
  a = (ip_repr >> 24);
  b = (ip_repr >> 16) & 0xFF;
  c = (ip_repr >> 8) & 0xFF;
  d = ip_repr & 0xFF;

  strm << a;
  strm << ".";
  strm << b;
  strm << ".";
  strm << c;
  strm << ".";
  strm << d;
  return strm.str();
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
