#include "Config.hpp"
#include "ConnectionManager.hpp"

#include <iostream>
#include <map>
#include <vector>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

ConnectionManager::ConnectionManager(Config &cfg) {
  config = cfg;
}

ConnectionManager::~ConnectionManager() {
  // FIX - write a destructor
}

int ConnectionManager::setup(const Config& config) {
  /* ----- Config init*/
  // FIX - load the config and use it
  (void)config;
  int port_ = 8080;
  // FIX - assign several listening sockets for different servers
  // FIX - handle errors!
  /* ----- Listening socket init*/
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
      perror("socket");
    return (-1);
  }
  struct sockaddr_in server_addr_in;
  server_addr_in.sin_family = AF_INET;
  server_addr_in.sin_addr.s_addr = INADDR_ANY; // FIX - add real IP that was in the config
  server_addr_in.sin_port = htons(port_);
  memset(&(server_addr_in.sin_zero), '\0', 8);

  int opt = 1;
  // FIX - handle errors!
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt");
    return (-1);
  }

  // FIX - handle errors!
  if (bind(listen_fd, (struct sockaddr *)&server_addr_in, sizeof(server_addr_in)) < 0) {
      perror("bind");
      return (-1);
  }


  std::cout << "I HAVE BINDED THE SERVER\n";
  // FIX - handle errors!
  // FIX - load data from the config!
  if (listen(listen_fd, 20) == -1)
	{
		perror("listen");
		return (-1);
	}
  this->listen_fds.push_back(listen_fd);
  struct pollfd poll_fd = {listen_fd, POLLIN, 0};
  fds.push_back(poll_fd);

  std::cout << "SERVER IS LISTENING\n";
  listen_fds.push_back(listen_fd);
  return 0;
}

int ConnectionManager::run(const Config& config) {
  (void)config;
  char buffer[CHUNK_SZ_FIX]; // FIX - buffer size should be dynamic depending on config or idk on something else
  (void)buffer;
  std::cout << "Starting select() loop" << std::endl;
  // FIX: timeout to poll is probly taken from the config
  while (true) {
    // FIX: set all poll revents to zero!
    int poll_result = poll(fds.data(), fds.size(), 10);
    if (poll_result == -1) {
      std::cout << "poll error" << std::endl; // FIX log
      exit(1);
    }
    if (poll_result == 0) {
      continue;
    }
    handle_fds(poll_result);
  }
  return 0;
}

int ConnectionManager::handle_fds(int fd_count) {
  (void)fd_count;
  for (int i = 0, sz = fds.size(); i < sz; i++) {
    if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
      if (handle_poll_problem(i)) {
        break;
      }
    }else if (fds[i].revents & POLLIN) {
      if (handle_poll_read(i)) {
        break;
      }
    } else if (fds[i].revents & POLLOUT) {
      if (handle_poll_write(i)) {
        break;
      }
    } else {
      //???
    }
    //handle_timeouts() ??
  }
}

int ConnectionManager::handle_poll_problem(int fd) {
  // if in listen_fds then handle_accept()
  std::cout << "ERROR" << std::endl; // FIX
}

int ConnectionManager::handle_poll_read(int fd) {
  std::cout << "READING" << std::endl; // FIX

}

int ConnectionManager::handle_poll_write(int fd) {
  std::cout << "WRITING" << std::endl; // FIX

}

int ConnectionManager::handle_accept(int fd) {
}