
#include "Config.hpp"
#include "Libft.hpp"

#include <iostream>

#include <arpa/inet.h>
#include <stdlib.h>


void cv_one(const std::string &s) {
  unsigned long l;
  unsigned long i;
  l = Config::string_to_ip(s);
  i = Libft::ft_atoip(s);
  std::cout << "Libft:" << std::endl;
  std::cout << "\tulong: [" << l << "]" << std::endl;
  std::cout << "\tint  : [" << i << "]" << std::endl;
  std::cout << "\ti->s : [" << Libft::ft_iptoa(i) << "]" << std::endl;
  std::cout << "\tul->s: [" << Libft::ft_iptoa(static_cast<int>(l)) << "]" << std::endl;
  std::cout << std::endl;

  unsigned long std_i;
  char *dst = (char *)calloc(35, 1);
  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_port = 80;
  bzero(sa.sin_zero, 8);
  inet_pton(AF_INET, s.c_str(), &std_i);
  sa.sin_addr.s_addr = std_i;

  if (inet_ntop(AF_INET, &(sa.sin_addr), dst, 35) == NULL) {
    std::cout << "ERROR\n";
  };
  std::cout << "std:::" << std::endl;
  std::cout << "\tulong: [" << ntohl(std_i) << "]" << std::endl;
  std::cout << "\tint  : [" << ntohl(std_i) << "]" << std::endl;
  std::cout << "\ti->s : [" << dst << "]" << std::endl;
  std::cout << "\tul->s: [" << dst << "]" << std::endl;

}

int main(){
  cv_one("127.0.0.1");
  cv_one("124.88.77.99");
}
