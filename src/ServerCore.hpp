#ifndef SERVER_CORE_HPP
#define SERVER_CORE_HPP

#include <string>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

class	ServerCore {
public:
  ServerCore();
  ServerCore(ServerCore const &other);
  ~ServerCore();
  ServerCore &operator=(const ServerCore &obj);

  ServerCore(const std::string &ip, int port, int listen_backlog);

  int setup();
  int run();


private:
  std::string ip_string_;
  int port_;
  int listen_backlog_;
  int listen_fd;
  sockaddr_in server_addr_in;
private:
  fd_set reading_set_;
  fd_set writing_set_;
  

};

#endif // SERVER_CORE_HPP
