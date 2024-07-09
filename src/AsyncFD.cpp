#include "AsyncFD.h"

#include "Config.hpp"
#include <iostream>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>



AsyncFD::AsyncFD() {}

AsyncFD::AsyncFD(AsyncFD const &other) : {};

AsyncFD::~AsyncFD() {}

AsyncFD	&AsyncFD::operator=(const AsyncFD &other) {
  if (this != &other) {
  }
  return (*this);
}


int AsyncFD::async_write(const std::string &msg) {
  write_buff.append(msg);
  int bytes_sent;
  if (type == FD_TYPE_SOCK) {
    bytes_sent = send(fd, write_buff.c_str(), write_buff.length(), 0);
  } else {
    bytes_sent = write(fd, write_buff.c_str(), write_buff.length());
  }
  write_buff.erase(0, bytes_sent);
}

int AsyncFD::async_read(std::string &buffer, int len) {

}


bool AsyncFD::is_write_buf_empty() {

}

bool AsyncFD::is_read_buf_empty() {

}
