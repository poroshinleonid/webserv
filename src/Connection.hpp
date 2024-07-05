#ifndef CONNECTION_HPP
#define CONNECTION_HPP

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

#define CHUNK_SZ_FIX 1024

class	Connection {
public:
  Connection();
  Connection(Connection const &other);
  ~Connection();
  Connection &operator=(const Connection &obj);

public:
  Connection(const std::string &ip, int port, int listen_backlog);

public:
  int setup(const Config& config);
  int run(const Config& config);

private:
  void handle_fds();
  void handle_read(int fd);
  void handle_accept(int fd);
  int handle_cgi_output(int cgi_pipe_fd);
  void handle_incoming_data(int fd);
  void handle_write(int fd);

  int handle_request_if_ready(int fd);

private:
  int accept_(); // accept new connection, returns new_fd. updates (or not?) read_buffers and write_buffers maps.
  int recv_chunk_(int fd); // recieves a chunk of data
  int send_chunk_(int fd); // send a chunk of data

/*
data structures to update:
  1. all_fds_
  2. max_fd_
  3. read/write buffer maps
  4. cgi fds
  5. cgi maps
*/

// static data, does not change
private:
  std::string ip_str_;
  int port_;
  int listen_backlog_;
  int listen_fd_;
  sockaddr_in server_addr_in;

// dynamic data, can change with each select() iteration
private:
  fd_set all_fds_;
  fd_set read_fds_;
  fd_set write_fds_;
  fd_set cgi_fds_;
  int max_fd_;

// dynamic data structures, can change with each select() iteration
private:
  std::map<int, std::string> read_buffers_;
  std::map<int, std::string> write_buffers_;
  std::map<int, int> cgi_pipes_to_sockets_;

};

#endif // CONNECTION_HPP
