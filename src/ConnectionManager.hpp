#pragma once

#include "Config.hpp"
#include "HttpConnection.hpp"
#include "HttpRequest.hpp"
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

#define BUF_SZ 8192

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
   * @brief Setup the webserv.
   *
   * Created Server instances for every server in the config.
   * Fills in all of the data for every server.
   * @return int
   * @retval 0 on success
   * @retval -1 if none of the servers were sucessfully set up
   */
  int setup();

  /**
   * @brief Start the server.
   *
   * @note Runs indefinitely.
   * @note Returns on fatal poll() error or if there ar no servers
   * @return int
   * @retval 0 on success
   * @retval 1 if there are no servers
   * @retval -1 on fatal error in poll()
   */
  int run();

private:
  /**
   * @brief Set the up server from cfg["server"]
   *
   * @see Server.hpp
   * @param serv
   * @param cfg
   * @return int
   */
  int setup_server(Server &serv, Config &cfg);

  /**
   * @brief Start Server instance
   *
   * Enable listening socket, add it to pollfd list
   * @param serv
   * @return int
   */
  int start_server(Server &serv);

  /**
   * @brief setup_server() and start_server()
   *
   * @param cfg
   * @return int
   */
  int add_listen_server(Config &cfg);

  /**
   * @brief check timeouts for a socket
   *
   * close the connection or kill the CGI if needed
   * @param fd
   * @return int
   */
  void cleanup(int fd);

  void  add_pollfd(struct pollfd fd, int poll_in_flag, int poll_out_flag);
  void  remove_pollfd(size_t index);

  /**
   * @brief Go through pollfd vector after poll() returned positive value
   *
   *
   * The main program loop calls it.
   * @return int
   * @retval 0 on success
   * @retval -1 on fatal error
   */
  int handle_fds();

  /**
   * @brief handle POLLERR and POLLNVAL
   *
   * @param fd
   * @return true
   * @return false
   */
  void handle_revent_problem(int fd);

  /**
   * @brief Process an fd that is ready to be read from
   *
   * Can be a pipe or a socket
   * @param fd
   * @return true
   * @return false
   */
  bool handle_poll_read(int fd);

  /**
   * @brief Process an fd that is ready to be written to
   *
   * @param fd
   * @return true
   * @return false
   */
  bool handle_poll_write(int fd);

  /**
   * @brief accept a listen socket and create
   * HttpConnection instance for the new connection
   *
   * @param fd
   */
  void handle_accept(int fd);

private:
  /**
   * @brief checks if CGI is saying something
   * and processes its output
   *
   * @param connection
   * @return true
   * @return false
   */
  bool handle_cgi_output(HttpConnection &connection);

  /**
   * @brief closes the connection, cleans up
   *
   * @param fd
   */
  void close_connection(int fd);

  /**
   * @brief checks if the connections[fd]'s CGI has timed out
   *
   * @param fd
   * @return true
   * @return false
   */
  bool conn_timed_out(int fd);

  /**
   * @brief
   *
   * @param fd
   * @return true
   * @return false
   */
  bool cgi_timed_out(int fd);

  /**
   * @brief takes fd of the socket, finds the associated pipe fd in the
   * structures without external help
   *
   * @param fd
   */
  void kill_cgi(int connection_fd, bool send_kill_sig);

  /**
   * @brief read a chunk of data from CGI
   *
   * if all read, set is_response_ready to true and kill the cgi
   * @param connection
   * @return true
   * @return false
   */
  bool read_cgi_pipe(HttpConnection &connection);

  /**
   * @brief kill CGI and prepare to send "CGI timeout" response to the client.
   *
   * @param connection_fd
   */
  void timeout_and_kill_cgi(int connection_fd, bool send_kill_sig);

  /**
   * @brief shutdown Server instance
   *
   * Correctly clears up all connections
   * closes the socket
   * removes listen_fd from pollfd vector
   * removes Server instance from the listen_servers
   * @param listen_fd
   */
  void shutdown_server(int listen_fd);

  /**
   * @brief shutdowns the webserv
   *
   */
  void shutdown();

  int handle_poll_error(int err_num);
  int find_fd_index(int system_fd);
  bool write_to_cgi(int fd);

  bool cgi_write(int cgi_pid);
  void kill_cgi_and_prep_to_send_500(int con_fd, bool send_kill_sig);




  // void sighandle(int signum);

private: // deprecated
  int run_cgi(HttpConnection &connection, HttpRequest &request);
  int exec_cgi(HttpConnection &connection, HttpRequest &request);
  std::string try_read_fork(HttpConnection &connection, HttpRequest &request);

  // private: // moved to HttpConnection
  //   void update_last_activity(HttpConnection &connection);
  //   void update_last_cgi_activity(HttpConnection &connection);

private:
  Config *config;
  Logger *logger;
  std::vector<struct pollfd> fds;// all poll() fds
  std::map<int, HttpConnection> connections;
  std::map<int, Server> listen_servers;
  std::map<int, int> read_fd_to_sock; // (read-from pipe of cgi -> sock_fd related to cgi)
  std::map<int, int> write_fd_to_sock; // (write-to pipe of cgi -> sock_fd related to cgi)

  // system buffers
  char buffer[BUF_SZ];
  std::vector<short> pollin;
  std::vector<short> pollout;

public:
  void print_connection_manager();
};
