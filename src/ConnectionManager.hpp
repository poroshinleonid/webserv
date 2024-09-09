#ifndef CONNECTIONMANAGER_HPP
#define CONNECTIONMANAGER_HPP

#include "Config.hpp"
#include "HttpConnection.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Logger.hpp"

#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <unistd.h>
#include <vector>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>

class ConnectionManager {
private:
  ConnectionManager();
  ConnectionManager(ConnectionManager const &other);
  ConnectionManager &operator=(const ConnectionManager &obj);

public:
  ConnectionManager(Config *cfg, Logger *log);
  ~ConnectionManager();

public:
  /**
   * @brief Setup the web server.
   * 
   * @return int 
   */
  int setup();

  /**
   * @brief Run the server.
   * 
   * @note Runs indefinitely.
   * @return int 
   */
  int run();

private:
  /**
   * @brief Set the up server from cfg["server"]
   * 
   * @param serv 
   * @param cfg 
   * @return int 
   */
  int setup_server(Server &serv, Config &cfg);
  int start_server(Server &serv);
  int add_listen_server(Config &cfg);
  int cleanup(int fd);
  int handle_fds();
  bool handle_poll_problem(int fd);
  bool handle_poll_read(int fd);
  bool handle_poll_write(int fd);
  void handle_accept(int fd);

private:
  bool handle_cgi_output(HttpConnection &connection);
  void close_connection(int fd);
  bool conn_timed_out(int fd);
  bool cgi_timed_out(int fd);

  /**
   * @brief takes fd of the socket, finds the associated pipe fd in the structures without external help
   * 
   * @param fd 
   */
  void kill_cgi(int connection_fd);
  bool read_cgi_pipe(HttpConnection &connection);

private:
  int run_cgi(HttpConnection &connection, HttpRequest &request);
  int exec_cgi(HttpConnection &connection, HttpRequest &request);
  std::string try_read_fork(HttpConnection &connection, HttpRequest &request);

// private:
//   void update_last_activity(HttpConnection &connection);
//   void update_last_cgi_activity(HttpConnection &connection);

private:
  // system data
  Config *config;
  Logger *logger;

  // all fds for poll()
  std::vector<struct pollfd> fds;

  // connections with clients
  std::map<int, HttpConnection> connections;

  // listening servers
  // std::vector<int> listen_fds;
  std::map<int, Server> listen_servers;

  // CGI data
  std::map<int, int> pipe_to_socket;

  // system buffers
  char buffer[4096];     // FIX is it the right size?
  char cgi_buffer[1024]; // FIX is it the right size?

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
//   int accept_(); // accept new connection, returns new_fd. updates (or not?)
//   read_buffers and write_buffers maps. int recv_chunk_(int fd); // recieves a
//   chunk of data int send_chunk_(int fd); // send a chunk of data

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
#define DEBUG
#ifdef DEBUG
public:
  void print_connection_manager();
#endif
};

#endif // CONNECTIONMANAGER_HPP
