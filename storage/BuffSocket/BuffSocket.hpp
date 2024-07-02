#include <string>
#include <string>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>


class BuffSocket {
 public:
  BuffSocket();
  BuffSocket(const int &fd);
  ~BuffSocket();
 public:
  int send_buff();
  void add_send_buff(const std::string &buff);
  int recv_buff();
  void add_recv_buff(const std::string &buff);
 private:
  int socket_fd;
  int buff_size;
  std::string recv_buffer;
  std::string send_buffer;
};