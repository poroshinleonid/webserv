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

#define TRU 1
#define FALS 0

bool sig_stop = false;

ConnectionManager::ConnectionManager(Config *cfg, Logger *log)
    : config(cfg), logger(log), fds({}) {
  Libft::ft_memset(buffer, BUF_SZ, 0);
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
    serv.host = cfg["host"].unwrap();
    serv.port = serv.srv_sockaddr.sin_port;

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
  try {
  serv.timeout =
      static_cast<double>(Libft::ft_atoi((*config)["timeout"].unwrap()));
  } catch (...) {
    serv.timeout = serv.default_timeout;
  }
  try {
  serv.cgi_timeout =
      static_cast<double>(Libft::ft_atoi((*config)["cgi_timeout"].unwrap()));
  } catch (...) {
    serv.cgi_timeout = serv.default_cgi_timeout;
  }
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
  Libft::ft_memset(&(server_addr_in.sin_zero), 8, 0);
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
  if (listen(new_listen_fd, 63) == -1) {
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
  struct pollfd poll_fd = {serv.listen_fd, POLLIN | POLLOUT, 0};
  fds.push_back(poll_fd);
  pollin.push_back(TRU);
  pollout.push_back(FALS);
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
        poll(fds.data(), fds.size(), 10);
    if (poll_result == -1) {
      return handle_poll_error(errno);
    }
    if (poll_result == 0) {
      continue;
    }
    if (handle_fds() != 0) { // -1 on fatal error, 0 on success
      return -1;
    }
  }
  return 0;
}

void ConnectionManager::cleanup(int fd) {
  (void)fd;
  if (read_fd_to_sock.find(fd) != read_fd_to_sock.end()) {
    return;
  }
  if (write_fd_to_sock.find(fd) != write_fd_to_sock.end()) {
    return;
  }
  if (connections[fd].is_cgi_running && cgi_timed_out(fd)) {
    timeout_and_kill_cgi(fd, true);
    return;
  }
  if (conn_timed_out(fd)) {
      close_connection(fd);
    return;
  }
  return;
}

int ConnectionManager::handle_fds() {
  bool io_happened = false;
  size_t pollvec_len = fds.size();
  for (size_t i = 0; i < pollvec_len; i++) {
    int fd = fds[i].fd;
    if (io_happened) {
      cleanup(fd);
    } else if (fds[i].revents & (POLLERR | POLLNVAL)) {
      handle_revent_problem(fd);
    } else if ((fds[i].revents & POLLIN) && pollin[i] == TRU) {
      io_happened = handle_poll_read(fd);
    } else if ((fds[i].revents & POLLOUT) && pollout[i] == TRU) {
      io_happened = handle_poll_write(fd);
    }
    if (pollvec_len != fds.size()) {
      // one of the fds was deleted
      // we have no way to knof if it is before or after
      // the current one so we restart the poll loop
      break;
    }
  }
  return 0;
}

void ConnectionManager::handle_revent_problem(int fd) {
  std::string fd_type = "Socket ";
  if (read_fd_to_sock.find(fd) != read_fd_to_sock.end()) {
    fd_type = "Pipe-read fd ";
  } else if (write_fd_to_sock.find(fd) != write_fd_to_sock.end()) {
    fd_type = "Pipe-write fd ";
  }
  if (fds[find_fd_index(fd)].revents & POLLHUP) {
    logger->log_error(fd_type + Libft::ft_itos(fd) +
                      " hung up incorrectly (POLLHUP).");
  } else if (fds[find_fd_index(fd)].revents & POLLNVAL) {
    logger->log_error(fd_type + Libft::ft_itos(fd) +
                      " is closed unexpectedly (POLLNVAL).");
    connections[fd].socket_closed = true;
  } else if (fds[find_fd_index(fd)].revents & POLLERR) {
    logger->log_error(fd_type + Libft::ft_itos(fd) + " failed (POLLERR).");
  }
  if (fd_type == "Socket ") {
    close_connection(fd);
    return;
  }
  
  int sock_fd;
  if (fd_type == "Pipe-read fd ") {
    sock_fd = read_fd_to_sock[fd];
  } else { // "Pipe-write fd "
    sock_fd = write_fd_to_sock[fd];
  }
    kill_cgi(sock_fd, true);
    close_connection(sock_fd);
}

bool ConnectionManager::handle_poll_read(int fd) {
  if (pollin[find_fd_index(fd)] == FALS) {
    return false;
  }
  fds[find_fd_index(fd)].revents = 0;
  // Check if bytes actually came from a CGI and not from a client
  if (read_fd_to_sock.find(fd) != read_fd_to_sock.end()) {
    return handle_cgi_output(connections[read_fd_to_sock[fd]]);
  }
  // If bytes came to a listen server, we need to accept()
  if (listen_servers.find(fd) != listen_servers.end()) {
    logger->log_info("Socket " + Libft::ft_itos(fd) + ": Accepting...");
    handle_accept(fd);
    return true;
  }
  HttpConnection &connection = connections[fd];
  // if cgi is still running, stop recv-ing from this socket until cgi is
  // either finished or timed out
  if (connection.is_cgi_running) {
    if (cgi_timed_out(fd)) {
      timeout_and_kill_cgi(fd, true);
    }
    return false;
  }
  // This should not happen
  if (connection.recv_done == true) {
    logger->log_info(
        "Socket " + Libft::ft_itos(fd) +
        ": there's something to read according to poll(), but recv_done=true "
        "which means that the previous request wasn't responed yet");
        pollin[find_fd_index(fd)] = FALS;
        pollout[find_fd_index(fd)] = FALS;
    return false;
  }

  Libft::ft_memset(buffer, sizeof(buffer), 0);
  int bytes_recvd = recv(fd, buffer, BUF_SZ - 1, 0);
  connection.update_last_activity();
  // No bytes recieved = client hung up gracefully.
  if (bytes_recvd == 0) {
    if (connection.is_keep_alive == false) {
      logger->log_info("Socket " + Libft::ft_itos(fd) + " hung up gracefully.");
      close_connection(fd);
    } else if (connection.is_cgi_running == false) {
      logger->log_info("Socket " + Libft::ft_itos(fd) +
                       " hung up gracefully, it was keep-alive");
      close_connection(fd);
    } else {
      logger->log_info("Socket " + Libft::ft_itos(fd) +
                       " hung up gracefully, and didn't wait for CGI to respond");
      close_connection(fd);
    }
    return true;
  }
  // Negative bytes recieved = error
  if (bytes_recvd < 0) {
    logger->log_error("Socket " + Libft::ft_itos(fd) +
                      ": recv failed, closing the connection.");
    close_connection(fd);
    return true;
  }
  logger->log_info("Socket " + Libft::ft_itos(fd) + ": Recieved " +
                   Libft::ft_itos(bytes_recvd) + " bytes");
  connection.recv_buffer.append(buffer);

  std::string response_string;
  response_string = get_responses_string(connection);
  // this means not the whole request wasn't recv-d yet
  if (response_string.empty() && connection.is_cgi_running == false) {
    return true;
  }
  connection.recv_done = true;
  pollin[find_fd_index(fd)] = FALS;
  if (response_string.empty() && connection.is_cgi_running == true) {
    pollout[find_fd_index(fd)] = FALS;
    struct pollfd pollfd_read_s = {connection.cgi_pipe[0], POLLIN | POLLOUT, 0};
    fds.push_back(pollfd_read_s);
    pollin.push_back(TRU);
    pollout.push_back(FALS);
    read_fd_to_sock[connection.cgi_pipe[0]] = fd;

    struct pollfd pollfd_write_s = {connection.cgi_pipe[1], POLLIN | POLLOUT, 0};
    fds.push_back(pollfd_write_s);
    pollin.push_back(FALS);
    pollout.push_back(TRU);
    write_fd_to_sock[connection.cgi_pipe[1]] = fd;
    logger->log_info("Socket " + Libft::ft_itos(fd) + ": established cgi pipes: write to cgi in fd " + Libft::ft_itos(connection.cgi_pipe[1]) + ", read from cgi from fd " + Libft::ft_itos(connection.cgi_pipe[0]));
    connection.update_last_cgi_activity();
    return true;
  }
  pollout[find_fd_index(fd)] = TRU;
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
  new_pollfd_struct.events = POLLIN | POLLOUT;
  fds.push_back(new_pollfd_struct);
  pollin.push_back(TRU);
  pollout.push_back(FALS);
  HttpConnection connection(config, logger, &listen_servers[fd]);
  connection.fd = new_fd;
  connections[new_fd] = connection;
  connection.update_last_activity();
}

bool ConnectionManager::handle_poll_write(int fd) {
  if (pollout[find_fd_index(fd)] == FALS) {
    return false;
  }
  if (write_fd_to_sock.find(fd) != write_fd_to_sock.end()) {
    return write_to_cgi(fd);
  }
  fds[find_fd_index(fd)].revents = 0;
  // if the connection timed out, we just close it
  if (conn_timed_out(fd)) {
    close_connection(fd);
    return false;
  }
  // if the cgi timed out, we kill it and are ready to recieve
  // FIX - prepare to send 500!
  if (connections[fd].is_cgi_running) {
    if (cgi_timed_out(fd)) {
      timeout_and_kill_cgi(fd, true);
      pollout[find_fd_index(fd)] = FALS;
      pollin[find_fd_index(fd)] = TRU;
    }
    return false;
  }
  // We have nothing to send. Should not happen.
  if (connections[fd].send_buffer.empty()) {
    logger->log_error("Something wrong, this shouldn't happen. Nothing to send "
                      "but poll() searched for pollout");
    pollin[find_fd_index(fd)] = FALS;
    pollout[find_fd_index(fd)] = FALS;
    return false;
  }
  int bytes_sent = send(fd, connections[fd].send_buffer.c_str(),
                        connections[fd].send_buffer.length(), 0);
  #ifdef DEBUG
  std::cout << "Normal response sent: " << connections[fd].send_buffer
            << std::endl;
  #endif
  connections[fd].update_last_activity();
  // bytes_sent < 0 means error
  if (bytes_sent < 0) {
    logger->log_error("send failed on socket" + Libft::ft_itos(fd));
    close_connection(fd);
    return true;
  } else if (bytes_sent == 0) { // nothing
    return true;
  }
  logger->log_info("Socket " + Libft::ft_itos(fd) + ": Sent " +
                   Libft::ft_itos(bytes_sent) + " bytes");
  connections[fd].send_buffer.erase(0, bytes_sent);
  // If we sent everything we needed, we are ready to recieve again
  if (connections[fd].send_buffer.empty()) {
    if (connections[fd].close_after_send == true || connections[fd].is_keep_alive == false) {
      close_connection(fd);
      return true;
    }
    pollout[find_fd_index(fd)] = FALS;
    pollin[find_fd_index(fd)] = TRU;
    connections[fd].recv_done = false;
    connections[fd].is_chunked_transfer = false;
  }
  return true;
}

//rewrite based on write_to_cgi
bool ConnectionManager::handle_cgi_output(HttpConnection &connection) {
  // if the cgi has already been set to finished, read from the pipe
  if (connection.cgi_finished) {
    return read_cgi_pipe(connection);
  }

  int status_code;
  connection.cgi_result = waitpid(connection.cgi_pid, &status_code, WNOHANG);
  if (connection.cgi_result < 0) {
    kill_cgi_and_prep_to_send_500(connection.fd, true);
    return false;
  }
  if (connection.cgi_result == 0) { // CGI stil running
    return read_cgi_pipe(connection);
  }
  if (connection.cgi_result != connection.cgi_pid && connection.cgi_result != 0) {
    logger->log_error("Socket " + Libft::ft_itos(connection.fd) +
                      ": Waitpid returned garbage pid from cgi pipe #" +
                      Libft::ft_itos(connection.fd));
    kill_cgi_and_prep_to_send_500(connection.fd, true);
  }

  //If the CGI was terminated by exit()
  if (WIFEXITED(status_code)) {
    if (WEXITSTATUS(status_code) != 0) { // exit(0)
      if (connection.send_buffer.empty()) {
        // CGI exited before giving us a response
        kill_cgi_and_prep_to_send_500(connection.fd, false);
        return false;
      } else {
        // CGI exited and we have a response
        kill_cgi(connection.fd, false);
        connection.is_response_ready = true;
        pollout[find_fd_index(connection.fd)] = TRU;
        pollin[find_fd_index(connection.fd)] = FALS;
        return false;
      }
    } else { //exit() != 0, prepare 500
      kill_cgi_and_prep_to_send_500(connection.fd, false); 
      return false;
    }
  } else {// !WIFEXITED(status_code) exit by signal or & C-Z in terminal
    if (WIFSIGNALED(status_code)) {
      logger->log_warning("CGI: exited with a signal" + Libft::ft_itos(WTERMSIG(status_code)));
      kill_cgi_and_prep_to_send_500(connection.fd, false);
    } else if (WIFSTOPPED(status_code)) {
      logger->log_warning("CGI: WIFSTOPPED=true");
      kill_cgi_and_prep_to_send_500(connection.fd, true);
    } else if (WIFCONTINUED(status_code)) {
      logger->log_warning("CGI: WIFCONTINUED=true");
      kill_cgi_and_prep_to_send_500(connection.fd, true);
    } else {
      logger->log_warning("CGI: !WIFEXITED - unknown error");
      kill_cgi_and_prep_to_send_500(connection.fd, true);
    }
    return false;
  } 
}

// assumes poll() said we can read from the pipe!
//rework like write_to_cgi
bool ConnectionManager::read_cgi_pipe(HttpConnection &connection) {
  int buf_len = sizeof(buffer);
  logger->log_info("Socket " + Libft::ft_itos(connection.fd) +
                   ": reading CGI pipe " +
                   Libft::ft_itos(connection.cgi_pipe[0]));
  int bytes_read;
  connection.update_last_cgi_activity();
  Libft::ft_memset(buffer, buf_len, 0);
  bytes_read = read(connection.cgi_pipe[0], buffer, buf_len - 1);
  if (bytes_read < 0) {
    logger->log_error("Socket " + Libft::ft_itos(connection.fd) +
                      ": read failed on CGI pipe " +
                      Libft::ft_itos(connection.cgi_pipe[0]));
    kill_cgi_and_prep_to_send_500(connection.fd, connection.is_cgi_running);
    return true;
  } else if (bytes_read == buf_len) { // read success, there's some more to read.
    connection.send_buffer.append(buffer);
    return true;
  } else if (bytes_read == 0) {
    connection.send_buffer.append(buffer);
    size_t index = connection.send_buffer.find("Status: ");
    if (index != std::string::npos) {
      connection.send_buffer.replace(index, 7, "HTTP/1.1 ", 9);
    }
    if (connection.send_buffer.size() == 0) {  // CGI returned nothing for some reason
      kill_cgi_and_prep_to_send_500(connection.fd, connection.is_cgi_running);
      return true;
    }
  }  else { // bytes_read < buf_len
    connection.send_buffer.append(buffer);
    size_t index = connection.send_buffer.find("Status: ");
    if (index != std::string::npos) {
      connection.send_buffer.replace(index, 7, "HTTP/1.1 ", 9);
    }
  }
  std::cout << "Finished reading from CGI pipe, killing it after reading " \
            << connection.send_buffer.size() << " bytes" << std::endl;
  kill_cgi(connection.fd, true);
  connection.is_cgi_running = false;
  connection.cgi_finished = true;
  connection.is_response_ready = true;
  pollout[find_fd_index(connection.fd)] =TRU;
  pollin[find_fd_index(connection.fd)] =FALS;
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
    logger->log_warning("closing the connection so killling the CGI");
    kill_cgi(fd, true);
  }
  for (size_t i = 0, ie = fds.size(); i < ie; ++i) {
    if (fds[i].fd == fd) {
      fds.erase(fds.begin() + i);
      pollin.erase(pollin.begin() + i);
      pollout.erase(pollout.begin() + i);
      break;
    }
  }
  connections.erase(fd);
  logger->log_info("Socket " + Libft::ft_itos(fd) + ": Closed the connection");
}

bool ConnectionManager::conn_timed_out(int fd) {
  (void)fd;
  return false;
  time_t now;
  now = std::time(&now);
  double connection_age = std::difftime(now, connections[fd].last_activity);
  if (connection_age > connections[fd].serv->timeout) {
    return true;
  }
  return false;
}

bool ConnectionManager::cgi_timed_out(int fd) {
  (void)fd;
  return false;
  time_t now;
  now = std::time(&now);
  double cgi_age = std::difftime(now, connections[fd].last_activity);
  if (cgi_age > connections[fd].serv->timeout) {
    return true;
  }
  return false;
}

void ConnectionManager::kill_cgi(int connection_fd, bool send_kill_sig) {
  if (send_kill_sig) {
    kill(connections[connection_fd].cgi_pid, SIGKILL);
  }
  int read_pipe = connections[connection_fd].cgi_pipe[0];
  int write_pipe = connections[connection_fd].cgi_pipe[1];

  close(read_pipe);
  close(write_pipe);
  
  pollin.erase(pollin.begin() + find_fd_index(read_pipe));
  fds.erase(fds.begin() + find_fd_index(read_pipe));

  pollin.erase(pollin.begin() + find_fd_index(write_pipe));
  fds.erase(fds.begin() + find_fd_index(write_pipe));

  read_fd_to_sock.erase(connections[connection_fd].cgi_pipe[0]);
  write_fd_to_sock.erase(connections[connection_fd].cgi_pipe[1]);
  connections[connection_fd].is_cgi_running = false;
  // pollout[find_fd_index(connection_fd)] =FALS;
  // pollin[find_fd_index(connection_fd)] =TRU;

  logger->log_info("Killed CGI: Socket " + Libft::ft_itos(connection_fd) + ", read fd: " + Libft::ft_itos(read_pipe) + ", write fd: " + Libft::ft_itos(write_pipe));
}

void ConnectionManager::timeout_and_kill_cgi(int connection_fd, bool send_kill_sig) {
  logger->log_warning("CGI timed out, killing " + Libft::ft_itos(connection_fd));
  kill_cgi_and_prep_to_send_500(connection_fd, send_kill_sig);
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
  for (size_t i = 0, ie = fds.size(); i < ie; ++i) {
    if (fds[i].fd == listen_fd) {
      fds.erase(fds.begin() + i);
      pollin.erase(pollin.begin() + i);
      pollout.erase(pollout.begin() + i);
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


bool ConnectionManager::cgi_write(int cgi_pid) {
  HttpConnection &connection = connections[write_fd_to_sock[cgi_pid]];
  connection.update_last_cgi_activity();
  int bytes_written = write(cgi_pid, connection.cgi_write_buffer.c_str(), connection.cgi_write_buffer.size());
  std::cerr << "Written: " << bytes_written << std::endl;
  if (bytes_written < 0) {
    logger->log_error("Can't send data to pipe" + Libft::ft_itos(cgi_pid));
    kill_cgi_and_prep_to_send_500(connection.fd, true);
  } else if (bytes_written == 0 || connection.cgi_write_buffer.size() == 0) {
    //nothing to write to cgi, stop poll()ing the write pipe
    pollout[find_fd_index(connection.cgi_pipe[1])] = FALS;
  } else if (bytes_written < static_cast<int>(connection.cgi_write_buffer.size())) {
    connection.cgi_write_buffer.erase(0, bytes_written);
  } else { // bytes_written < BUF_SZ
    connection.cgi_write_buffer.clear();
    pollout[find_fd_index(connection.cgi_pipe[1])] = FALS;
  }
  return true;
}

void ConnectionManager::kill_cgi_and_prep_to_send_500(int con_fd, bool send_kill_sig) {
  HttpConnection &connection = connections[con_fd];
  kill_cgi(con_fd, send_kill_sig);
  connection.send_buffer = status_code_to_string(500);
  connection.is_cgi_running = false;
  connection.cgi_finished = true;
  connection.is_response_ready = true;
  pollout[find_fd_index(con_fd)] = TRU;
  pollin[find_fd_index(con_fd)] = FALS;
}

bool ConnectionManager::write_to_cgi(int cgi_pid) {
  HttpConnection &connection = connections[write_fd_to_sock[cgi_pid]];
  int con_fd = connection.fd;
  connection.update_last_cgi_activity();
  int status_code;
  connection.cgi_result = waitpid(connection.cgi_pid, &status_code, WNOHANG);
  if (connection.cgi_result < 0) {
    kill_cgi_and_prep_to_send_500(con_fd, true);
    return false;
  }
  if (connection.cgi_result == 0) { // CGI stil running
    return cgi_write(cgi_pid);
  }
  if (connection.cgi_result != connection.cgi_pid) {
    logger->log_error("Socket " + Libft::ft_itos(connection.fd) +
                      ": Waitpid returned garbage pid from cgi pipe #" +
                      Libft::ft_itos(connection.fd));
    kill_cgi_and_prep_to_send_500(con_fd, true);
    return false;
  }

  //now the only remaining cgi_result is cgi_pid
  //which means the CGI is no loger running. 
  //Find out why and act accordingly


  //If the CGI was terminated by exit()
  if (WIFEXITED(status_code)) {
    if (WEXITSTATUS(status_code) != 0) { // exit(0)
      if (connection.send_buffer.empty()) {
        // CGI exited before giving us a response
        kill_cgi_and_prep_to_send_500(con_fd, false);
        return false;
      } else {
        // CGI exited and we have a response
        kill_cgi(con_fd, false);
        connection.is_response_ready = true;
        pollout[find_fd_index(con_fd)] = TRU;
        pollin[find_fd_index(con_fd)] = FALS;
        return false;
      }
    } else { 
      kill_cgi_and_prep_to_send_500(con_fd, false); //exit() != 0, prepare 500
      return false;
    }
  } else { // !WIFEXITED(status_code), exit by signal or & C-Z in terminal
    if (WIFSIGNALED(status_code)) {
      logger->log_warning("CGI: exited with a signal" + Libft::ft_itos(WTERMSIG(status_code)));
      kill_cgi_and_prep_to_send_500(con_fd, false);
    } else if (WIFSTOPPED(status_code)) {
      logger->log_warning("CGI: WIFSTOPPED=true");
      kill_cgi_and_prep_to_send_500(con_fd, true);
    } else if (WIFCONTINUED(status_code)) {
      logger->log_warning("CGI: WIFCONTINUED=true");
      kill_cgi_and_prep_to_send_500(con_fd, true);
    } else {
      logger->log_warning("CGI: !WIFEXITED - unknown error");
      kill_cgi_and_prep_to_send_500(con_fd, true);
    }
    return false;
  }
}



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

  std::cout << "read_fd_to_sock list:" << std::endl;
  for (std::map<int, int>::iterator it = read_fd_to_sock.begin();
       it != read_fd_to_sock.end(); ++it) {
    std::cout << "\t" << it->first << ": ";
    std::cout << it->second;
    std::cout << std::endl;
  }
  std::cout << std::endl;
  std::cout << "====\n====\n====\n";
}