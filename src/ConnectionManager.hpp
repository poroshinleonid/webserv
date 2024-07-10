#ifndef CONNECTIONMANAGER_HPP
#define CONNECTIONMANAGER_HPP

#include "Config.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "HttpConnection.hpp"

#include <iostream>
#include <map>
#include <vector>
#include <unistd.h>
#include <cstring>
#include <string>

#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>


#define CHUNK_SZ_FIX 1024

class	ConnectionManager {
public:
  ConnectionManager();
  ConnectionManager(ConnectionManager const &other);
  ~ConnectionManager();
  ConnectionManager &operator=(const ConnectionManager &obj);

//  public:
//   ConnectionManager(const std::string &ip, int port, int listen_backlog);


public:
  ConnectionManager(Config &cfg);


public:
  int setup(const Config& config);
  int run(const Config& config);

  int handle_fds(int fd_count);
  int handle_poll_problem(int fd);
  int handle_poll_read(int fd);
  int handle_poll_write(int fd);
  int handle_accept(int fd);


private:
  Config &config;
  std::vector<struct pollfd> fds;
  std::map<int, HttpConnection> connections;
  std::vector<int> listen_fds;





// private:
//   void handle_fds();
//   void handle_read(int fd);
//   void handle_accept(int fd);
//   int handle_cgi_output(int cgi_pipe_fd);
//   void handle_incoming_data(int fd);
//   int handle_write(int fd);

//   int handle_request_if_ready(int fd);
//   void check_timeouts(int fd);

// private:
//   int accept_(); // accept new connection, returns new_fd. updates (or not?) read_buffers and write_buffers maps.
//   int recv_chunk_(int fd); // recieves a chunk of data
//   int send_chunk_(int fd); // send a chunk of data

// /*
// data structures to update:
//   1. all_fds_
//   2. max_fd_
//   3. read/write buffer maps
//   4. cgi fds
//   5. cgi maps
// */

// // static data, does not change
// private:
//   std::string ip_str_;
//   int port_;
//   int listen_backlog_;
//   int listen_fd_;
//   sockaddr_in server_addr_in;

// // dynamic data, can change with each select() iteration
// private:
//   fd_set all_fds_;
//   fd_set read_fds_;
//   fd_set write_fds_;
//   fd_set cgi_fds_;
//   int max_fd_;

// // dynamic data structures, can change with each select() iteration

};

#endif // CONNECTIONMANAGER_HPP
