#include "ConnectionManager.hpp"
#include "Base.hpp"
#include "Config.hpp"
#include "HttpConnection.hpp"
#include "HttpHandle.hpp"
#include "HttpRequest.hpp"
#include "Libft.hpp"
#include "Server.hpp"

#include <algorithm>
#include <cstring>
#include <future>
#include <iostream>
#include <map>
#include <sstream>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <ctime>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define recv_chunk_sz 32768

bool sig_stop = false;

ConnectionManager::ConnectionManager(Config *cfg, Logger *log)
    : config(cfg), logger(log) {
  bzero(buffer, sizeof(buffer));
  bzero(cgi_buffer, sizeof(cgi_buffer));
}

ConnectionManager::~ConnectionManager() {
  for (size_t i = 0; i < fds.size(); i++) {
    close(i);
  }
}

int ConnectionManager::setup_server(Server &serv, Config &cfg) {
  try {
    serv.srv_sockaddr.sin_family = AF_INET;
    std::string port_str = cfg.get_value_safely("listen");
    if (port_str.empty()) {
      serv.srv_sockaddr.sin_port = 80;
    } else {
      serv.srv_sockaddr.sin_port = Libft::ft_atoi(port_str);
    }
    serv.srv_sockaddr.sin_addr.s_addr =
        Config::string_to_ip(cfg["host"].unwrap());
    Libft::ft_memset(serv.srv_sockaddr.sin_zero, 8, 0);
    serv.host = serv.srv_sockaddr.sin_addr.s_addr;
    serv.port = serv.srv_sockaddr.sin_port;

    // 2. Setup the server_names or not.
    // 3. The first server for a host:port will be the default for this
    // host:port (that means it will answer to all the requests that don’t
    // belong to an other server). std::istringstream
    // iss(cfg["server_name"].unwrap()); std::string srv_name; while (iss >>
    // srv_name) {
    //   serv.server_names.push_back(srv_name);
    // }
    serv.server_names =
        Config::split_string(cfg.get_value_safely("server_name"));
    serv.client_max_body_size =
        Libft::ft_atoi(cfg.get_value_safely("client_max_body_size"));
    if (serv.client_body_size == 0) {
      serv.client_body_size = Config::client_default_max_body_size;
    }
    return 0;
  } catch (std::exception &e) {
    logger->log_error("Can't setup one of the servers: " +
                      static_cast<std::string>(e.what()));
    return (-1);
  };
  return 0;
}

int ConnectionManager::start_server(Server &serv) {
  // get value safely
  serv.timeout =
      static_cast<double>(Libft::ft_atoi((*config)["timeout"].unwrap())) /
      1000.0;
  serv.cgi_timeout =
      static_cast<double>(Libft::ft_atoi((*config)["cgi_timeout"].unwrap())) /
      1000.0;
  int new_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (new_listen_fd < 0) {
    std::string error = "socket(): can't create socket " + serv.host + ":" +
                        Libft::ft_itos(serv.port) + " : " + strerror(errno);
    logger->log_error(error);
    return (-1);
  }
  struct sockaddr_in server_addr_in;
  server_addr_in.sin_family = serv.srv_sockaddr.sin_family;
  server_addr_in.sin_addr.s_addr = htonl(serv.srv_sockaddr.sin_addr.s_addr);
  server_addr_in.sin_port = htons(serv.port);
  memset(&(server_addr_in.sin_zero), '\0', 8); // LIBFT bzero
  int opt = 1;
  if (setsockopt(new_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    std::string error = "setsockopt(): can't assign address to socket" +
                        serv.host + ":" + Libft::ft_itos(serv.port) + " : " +
                        strerror(errno);
    logger->log_error(error);
    close(new_listen_fd);
    return (-1);
  }
  if (bind(new_listen_fd, (struct sockaddr *)&server_addr_in,
           sizeof(server_addr_in)) < 0) {
    std::string error = "bind(): can't bind socket " + serv.host + ":" +
                        Libft::ft_itos(serv.port) + " : " + strerror(errno);
    logger->log_error(error);
    close(new_listen_fd);
    return (-1);
  }
  // REVISE - load data about listen backlog from the config or from somewhere!
  if (listen(new_listen_fd, 20) == -1) {
    std::string error = "can't listen to socket" + serv.host + ":" +
                        Libft::ft_itos(serv.port) + " : " + strerror(errno);
    logger->log_error(error);
    close(new_listen_fd);
    return (-1);
  }
  std::string status_message = "Server " + serv.host + ":" +
                               Libft::ft_itos(serv.port) + " is listening!";
  logger->log_info(status_message);
  serv.listen_fd = new_listen_fd;
  listen_servers[serv.listen_fd] = serv;
  struct pollfd poll_fd = {serv.listen_fd, POLLIN, 0};
  fds.push_back(poll_fd);
  return 0;
}

int ConnectionManager::add_listen_server(Config &cfg) {
  Server serv;
  if (setup_server(serv, cfg) == -1) {
    return -1;
  }
  return start_server(serv);
}

void sighandle(int signum) {
  (void)signum;
  sig_stop = true;
}

int ConnectionManager::setup() {
  signal(SIGINT, sighandle);
  std::vector<Config> server_configs;
  server_configs = config->get_vec("server");
  for (size_t i = 0; i < server_configs.size(); i++) {
    add_listen_server(server_configs[i]);
  }
  if (listen_servers.empty()) {
    logger->log_error("No servers were successfully set up!");
    return -1;
  }
  listen_servers[fds[0].fd].is_default = true;
  return 0;
}

int ConnectionManager::run() {
  if (listen_servers.empty()) {
    logger->log_error("No servers to run! Exitting.");
    return 1;
  }
  while (!sig_stop) {
    int poll_result =
        poll(fds.data(), fds.size(), 10); // REVISE:timeout from cfg?
    if (poll_result == -1) {
      return handle_poll_error(errno); // REVISE where is terminate()?
    }
    if (poll_result == 0) {
      continue;
    }
    if (handle_fds() != 0) { // -1 on fatal error, 0 on success
      return -1;
    }
  }
  // shutdown();
  return 0;
}

int ConnectionManager::cleanup(int fd) {
  (void)fd;
  // if (connections[fd].is_cgi_running && cgi_timed_out(fd)) {
  //   timeout_cgi(fd);
  //   return 0;
  // }
  // if (conn_timed_out(fd)) {
  //     close_connection(fd);
  //   return 1;
  // }
  return 0;
}

// Ensure handle_* functions return values are correct
int ConnectionManager::handle_fds() {
  bool io_happened = false;
  size_t pollvec_len = fds.size();
  for (size_t i = 0; i < pollvec_len; i++) {
    int fd = fds[i].fd;
    if (io_happened) {
      cleanup(fd);
    } else if (fds[i].revents & (POLLERR | POLLNVAL)) {
      handle_revent_problem(fd);
    } else if (fds[i].revents & POLLIN) {
      io_happened = handle_poll_read(fd);
    } else if (fds[i].revents & POLLOUT) {
      io_happened = handle_poll_write(fd);
    }
    pollvec_len = fds.size();
  }
  return 0;
}

void ConnectionManager::handle_revent_problem(int fd) {
  if (fds[find_fd_index(fd)].revents & POLLHUP) {
    logger->log_error("Socket " + Libft::ft_itos(fd) +
                      " hung up incorrectly (POLLHUP).");
  } else if (fds[find_fd_index(fd)].revents & POLLNVAL) {
    logger->log_error("Socket " + Libft::ft_itos(fd) +
                      " is closed unexpectedly (POLLINVAL).");
    connections[fd].socket_closed = true;
  } else if (fds[find_fd_index(fd)].revents & POLLERR) {
    logger->log_error("Socket " + Libft::ft_itos(fd) + " failed (POLLERR).");
  }
  // fds[find_fd_index(fd)].revents = 0;
  close_connection(fd);
}

bool ConnectionManager::handle_poll_read(int fd) {
  fds[find_fd_index(fd)].revents = 0;
  // Check if bytes actually came from a CGI and not from a client
  if (pipe_to_socket.find(fd) != pipe_to_socket.end()) {
    return handle_cgi_output(connections[pipe_to_socket[fd]]);
  }
  // If bytes came to a listen server, we need to accept()
  if (listen_servers.find(fd) != listen_servers.end()) {
    logger->log_info("Socket " + Libft::ft_itos(fd) + ": Accepting...");
    handle_accept(fd);
    return true;
  }
  HttpConnection &connection = connections[fd];
  // if cgi is still running, stop recv()ing from this socket until cgi is
  // either finished or timed out
  if (connection.is_cgi_running) {
    if (cgi_timed_out(fd)) {
      timeout_and_kill_cgi(fd);
    }
    return false;
  }
  // This should not ever happen
  if (connection.recv_done == true) {
    logger->log_info(
        "Socket " + Libft::ft_itos(fd) +
        ": there's something to read according to poll(), but recv_done=true "
        "which means that the previous request wasn't responed yet");
        
    return false;
  }

  char bufg[recv_chunk_sz + 1];
  Libft::ft_memset(bufg, recv_chunk_sz + 1, 0);
  int bytes_recvd = recv(fd, bufg, recv_chunk_sz, 0);
  connection.update_last_activity();
  // No bytes recieved = cliend hangs up gracefully.
  if (bytes_recvd == 0) {
    if (connection.is_keep_alive == false) {
      logger->log_info("Socket " + Libft::ft_itos(fd) + " hung up gracefully.");
      close_connection(fd);
    } else if (connection.is_cgi_running == false) {
      logger->log_info("Socket " + Libft::ft_itos(fd) +
                       " hung up gracefully, although it is keep-alive");
      close_connection(fd);
    } else {
      logger->log_info("Socket " + Libft::ft_itos(fd) +
                       " sent 0 bytes but my CGI is still running.");
      fds[find_fd_index(fd)].events &= POLLOUT;
    }
    return true;
  }
  // Negative bytes recieved = error
  if (bytes_recvd < 0) {
    logger->log_error("Socket " + Libft::ft_itos(fd) +
                      ": recv() failed, closing the connection.");
    close_connection(fd);
    return true;
  }
  // Recieved as many bytes as the buffer has = not the whole request is read
  // yet
  if (bytes_recvd == recv_chunk_sz) {
    connection.recv_stream << bufg;
    return true;
  }

  // Finished recieving, bytes_recvd < recv_chunk_sz
  logger->log_info("Socket " + Libft::ft_itos(fd) + ": Recieved " +
                   Libft::ft_itos(bytes_recvd) + " bytes");
  connection.recv_stream << bufg;
  std::string response_string = get_responses_string(connection);
  // this means not the whole request was recv()-d
  if (response_string.empty() && connection.is_cgi_running == false) {
    return true;
  }
  connection.recv_done = true;
  fds[find_fd_index(fd)].events = POLLOUT;
  if (response_string.empty() && connection.is_cgi_running == true) {
    struct pollfd new_pollfd_struct = {connection.cgi_pipe[0], POLLIN, 0};
    fds.push_back(new_pollfd_struct);
    pipe_to_socket[connection.cgi_pipe[0]] = fd;
    std::cerr << "CGI PIPE ON FD " << Libft::ft_itos(connection.cgi_pipe[0]) << ", Socket: " << fd << std::endl;
  }
  connection.send_buffer.append(response_string);
  return true;
}

void ConnectionManager::handle_accept(int fd) {
  sockaddr_in socket_address;
  socklen_t socket_address_length = sizeof(socket_address);
  int new_fd = accept(fd, (sockaddr *)&socket_address, &socket_address_length);
  int optval = 1;
  if (new_fd == -1) {
    logger->log_error("Failed to accept a new connection on socket" +
                      Libft::ft_itos(fd));
    return;
  }
  if (setsockopt(new_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) {
    logger->log_error("Socket " + Libft::ft_itos(new_fd) +
                      "Failed to set SO_REUSEADDR");
    return;
  }
  if (setsockopt(new_fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int)) < 0) {
    logger->log_error("Socket " + Libft::ft_itos(new_fd) +
                      "Failed to set SO_KEEPALIVE");
    return;
  }
  logger->log_info("Socket " + Libft::ft_itos(fd) +
                   ": Accepted new connection " + Libft::ft_itos(new_fd));

  struct pollfd new_pollfd_struct;
  new_pollfd_struct.fd = new_fd;
  new_pollfd_struct.events = POLLIN;
  fds.push_back(new_pollfd_struct);
  HttpConnection connection(config, logger, &listen_servers[fd]);
  connection.fd = new_fd;
  connections[new_fd] = connection;
  connection.update_last_activity();
}

bool ConnectionManager::handle_poll_write(int fd) {
  fds[find_fd_index(fd)].revents = 0;
  // if the connection timed out, we just close it
  if (conn_timed_out(fd)) {
    close_connection(fd);
    return false;
  }
  // if the cgi timed out, we kill it and are ready to recieve
  if (connections[fd].is_cgi_running) {
    if (cgi_timed_out(fd)) {
      timeout_and_kill_cgi(fd);
      fds[find_fd_index(fd)].events = POLLIN;
    }
    return false;
  }
  // We have nothing to send. Should not happen ever.
  if (connections[fd].send_buffer.empty()) {
    logger->log_error("Something wrong, this shouldn't happen. Nothing to send "
                      "but poll() searched for pollout");
    fds[find_fd_index(fd)].events = 0;
    return false;
  }
  int bytes_sent = send(fd, connections[fd].send_buffer.c_str(),
                        connections[fd].send_buffer.length(), 0);
  std::cout << "Normal response sent: " << connections[fd].send_buffer
            << std::endl;
  connections[fd].update_last_activity();
  // bytes_sent < 0 means error
  if (bytes_sent < 0) {
    logger->log_error("send failed on socket" + Libft::ft_itos(fd));
    close_connection(fd);
    return true;
  }
  logger->log_info("Socket " + Libft::ft_itos(fd) + ": Sent " +
                   Libft::ft_itos(bytes_sent) + " bytes");
  connections[fd].send_buffer.erase(0, bytes_sent);
  // If we sent everything we needed, we are ready to recieve again
  if (connections[fd].send_buffer.empty()) {
    fds[find_fd_index(fd)].events = POLLIN;
    connections[fd].recv_done = false;
  }
  return true;
}

bool ConnectionManager::handle_cgi_output(HttpConnection &connection) {
  // if the cgi has already finished, read from the pipe
  if (!connection.cgi_finished) {
    int status_code;
    connection.cgi_result = waitpid(connection.cgi_pid, &status_code, WNOHANG);
    // waitpid returned 0  means CGI is still running
    if (connection.cgi_result == 0) {
      return false;
    }
    // waitpid returned not cgi pid - should hever happen
    if (connection.cgi_result != connection.cgi_pid) {
      kill_cgi(connection.fd);
      logger->log_error("Socket " + Libft::ft_itos(connection.fd) +
                        ": Waitpid returned garbage pid from cgi pipe #" +
                        Libft::ft_itos(connection.fd));
      return false;
    }
    if (!WIFEXITED(status_code)) {
      kill_cgi(connection.fd);
      // FIX: this should probably be 502 internal error or something
      connection.send_buffer = "CGI ERROR";
      connection.is_cgi_running = false;
      connection.is_response_ready = true;
      return false;
    }
    // We are here if the cgi finished gracefully so we can read from its pipe
    connection.cgi_finished = true;
  }
  return read_cgi_pipe(connection);
}

// assumes poll() said we can read from the pipe!
bool ConnectionManager::read_cgi_pipe(HttpConnection &connection) {
  int buf_len = sizeof(cgi_buffer);
  logger->log_info("Socket " + Libft::ft_itos(connection.fd) +
                   ": reading CGI pipe " +
                   Libft::ft_itos(connection.cgi_pipe[0]));
  Libft::ft_memset(cgi_buffer, buf_len, 0);
  int bytes_read = read(connection.cgi_pipe[0], cgi_buffer, buf_len);
  connection.update_last_cgi_activity();
  connection.send_buffer.append(cgi_buffer);
  Libft::ft_memset(cgi_buffer, buf_len, 0);
  // FIX: INTERNAL SERVER ERROR should be a complete valid response.
  if (bytes_read < 0) {
    logger->log_error("Socket " + Libft::ft_itos(connection.fd) +
                      ": read() failed on CGI pipe " +
                      Libft::ft_itos(connection.cgi_pipe[0]));
    connection.send_buffer = "INTERNAL SERVER ERROR";
    connection.is_response_ready = true;
    kill_cgi(connection.fd);
    return true;
  }
  // Couldn't get all of the CGI output in one read
  if (bytes_read == buf_len) {
    return true;
  }
  // if we are here, the read from CGI pipe is finished
  kill_cgi(connection.fd);
  connection.cgi_finished = true;
  connection.is_response_ready = true;
  fds[connection.fd].events = POLLOUT;
  return true;
}

/*------------ UTILS ----------------*/

/**
 * @brief takes connection and correctly closes it, closes the sockets and pipes
 * etc
 *
 * @param fd
 */
void ConnectionManager::close_connection(int fd) {
  if (connections[fd].socket_closed == false) {
    close(fd);
  }
  if (connections[fd].is_cgi_running == true) {
    kill_cgi(fd);
  }
  for (std::vector<struct pollfd>::iterator it = fds.begin(); it != fds.end();
       ++it) {
    if (it->fd == fd) {
      fds.erase(it);
      break;
    }
  }
  connections.erase(fd);
  logger->log_info("Socket " + Libft::ft_itos(fd) + ": Closed the connection");
}

bool ConnectionManager::conn_timed_out(int fd) {
  time_t now;
  now = std::time(&now);
  double connection_age = std::difftime(now, connections[fd].last_activity);
  if (connection_age > connections[fd].serv->timeout) {
    return true;
  }
  return false;
}

bool ConnectionManager::cgi_timed_out(int fd) {
  time_t now;
  now = std::time(&now);
  double cgi_age = std::difftime(now, connections[fd].last_activity);
  if (cgi_age > connections[fd].serv->timeout) {
    return true;
  }
  return false;
}

void ConnectionManager::kill_cgi(int connection_fd) {
  kill(connections[connection_fd].cgi_pid, SIGKILL);
  close(connections[connection_fd].cgi_pipe[0]);
  pipe_to_socket.erase(connections[connection_fd].cgi_pipe[0]);
  connections[connection_fd].is_cgi_running = false;
  fds[find_fd_index(connection_fd)].events = POLLIN;
}

// FIX: "CGI TIMEOUT" should be a complete valid response
void ConnectionManager::timeout_and_kill_cgi(int connection_fd) {
  kill_cgi(connection_fd);
  connections[connection_fd].send_buffer = "CGI TIMEOUT";
  connections[connection_fd].is_cgi_running = false;
  connections[connection_fd].is_response_ready = true;
}

void ConnectionManager::shutdown_server(int listen_fd) {
  Server *srv = &(listen_servers[listen_fd]);
  for (auto it = connections.begin(); it != connections.end();) {
    if (it->second.serv == srv) {
      int fd_to_remove = it->second.fd;
      ++it;
      close_connection(fd_to_remove);
      continue;
    }
    ++it;
  }
  close(srv->listen_fd);
  for (auto it = fds.begin(); it != fds.end(); ++it) {
    if (it->fd == listen_fd) {
      fds.erase(it);
      break;
    }
  }
  listen_servers.erase(listen_fd);
}

void ConnectionManager::shutdown() {
  logger->log_info("Shutting down the server!");
  while (!listen_servers.empty()) {
    std::map<int, Server>::iterator it = listen_servers.begin();
    auto srv_to_shutdown = it->first;
    shutdown_server(srv_to_shutdown);
  }
}

int ConnectionManager::handle_poll_error(int err_num) {
  if (err_num == EFAULT) {
    logger->log_error("poll() error - EFAULT");
    return -1;
  } else if (err_num == EINTR) {
    shutdown();
    return 0;
  } else if (err_num == EINVAL) {
    logger->log_error("poll() error - EINVAL");
    return -1;
  } else if (err_num == ENOMEM) {
    logger->log_error("poll() error - ENOMEM");
    return -1;
  } else {
    logger->log_error("poll() error - Unknown");
    return -1;
  }
}

int ConnectionManager::find_fd_index(int system_fd) {
  for (size_t i = 0; i < fds.size(); i++) {
    if (fds[i].fd == system_fd) {
      return i;
    }
  }
  throw std::out_of_range(
      "The pollfd struct with the sought-after ->fd is not found");
}

#define DEBUG

#ifndef DEBUG
void ConnectionManager::print_connection_manager() {}
#endif

#ifdef DEBUG
void ConnectionManager::print_connection_manager() {
  std::cout << "====\n====\n====\n";
  std::cout << (*config).get_content() << std::endl;
  std::cout << logger << std::endl;
  std::cout << "File descriptor list:" << std::endl;

  for (size_t i = 0; i < fds.size(); i++) {
    std::cout << fds[i].fd << " ";
  }
  std::cout << std::endl;

  std::cout << "Connections list:" << std::endl;
  for (std::map<int, HttpConnection>::iterator it = connections.begin();
       it != connections.end(); ++it) {
    std::cout << "\t" << it->first << ": ";
    it->second.print_connection();
    // std::cout << std::endl;
  }
  std::cout << std::endl;

  std::cout << "listen_servers list:" << std::endl;
  for (std::map<int, Server>::iterator it = listen_servers.begin();
       it != listen_servers.end(); ++it) {
    std::cout << "\t{" << it->first << ": \n";
    it->second.print_server();
  }
  std::cout << "}" << std::endl;

  std::cout << "pipe_to_socket list:" << std::endl;
  for (std::map<int, int>::iterator it = pipe_to_socket.begin();
       it != pipe_to_socket.end(); ++it) {
    std::cout << "\t" << it->first << ": ";
    std::cout << it->second;
    std::cout << std::endl;
  }
  std::cout << std::endl;
  std::cout << "====\n====\n====\n";
}
#endif