#include "ConnectionManager.hpp"
#include "Config.hpp"
#include "HttpConnection.hpp"
#include "Server.hpp"
#include "Libft.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
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

bool sig_stop = false;

std::string get_responses_string(HttpConnection &connection) {
  std::string st = connection.recv_stream.str();
  connection.recv_stream.clear();
  // write(1, "REQ", 3);
  // std::cout << "REQUEST::::" << st << std::endl;
  std::string answ = "HTTP/1.1 200 OK\r\n";
  if (st.find("keep-alive") != string::npos) {
  // if (true) {
    connection.is_keep_alive = true;
    std::cout << "KEEP";
    answ += "Connection: keep-alive\r\n";
  }
  answ += "Content-Type: text/plain\r\n"
          "Content-Length: 12\r\n";
  answ += "\r\n"
         "Hello world!";
  // std::cout << connection.recv_stream.str();
  connection.recv_done = true;
  // std::cout << answ;
  return answ;
}

// std::string get_responses_string(HttpConnection &connection) {
//   std::string st = connection.recv_stream.str();
//   // if (st.find("keep-alive")) {
//   if (true) {
//     connection.is_keep_alive = true;
//   }
//   std::cout << "Request:\t" << st << std::endl;
//   (void)connection;
//   std::string resp = "HTTP/1.1 200 OK\r\n"
//          "Content-Type: text/plain\r\n"
//          "Content-Length: 12\r\n"
//          "\r\n"
//          "Hello, world!" + 
//          st;
//   return resp;
// }

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
    // 1. Choose the port and host of each ’server’.
    serv.srv_sockaddr.sin_family = AF_INET;
    serv.srv_sockaddr.sin_port = Libft::ft_atoi(cfg["port"].unwrap());
    serv.srv_sockaddr.sin_addr.s_addr =
        Config::string_to_ip(cfg["host"].unwrap());
    bzero(serv.srv_sockaddr.sin_zero, 8); // LIBFT: forbidden fun
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
    serv.server_names = Config::split_string(cfg.get_value_safely("server_name"));

    // 4. Setup default error pages.
    std::vector<Config> error_pages = cfg.get_vec("default_page");
    for (size_t i = 0; i < error_pages.size(); i++) {
      ErrorPage page;
      page.page_code = Libft::ft_atoi(error_pages[i]["code"].unwrap());
      page.html_file = error_pages[i]["html_file"].unwrap();
      page.location = error_pages[i]["location"].unwrap();
      std::string tmp = error_pages[i].get_value_safely("internal");
      page.internal =  tmp == "true"|| tmp == "";
      serv.error_pages.push_back(page);
    }

    // 5. Limit client body size.
    serv.client_max_body_size = Libft::ft_atoi(cfg.get_value_safely("client_max_body_size"));
    if (serv.client_body_size == 0) {
      serv.client_body_size = Config::client_default_max_body_size;
    }

    // 6. Setup routes with one or multiple of the following rules/configuration
    // 1. get_vec routes
    // 2. for each cfg in vector parse it and push_back the result to
    // Server.routes (which is a vector);
    std::vector<Config> routes_vec = cfg.get_vec("route");
    for (size_t i = 0; i < routes_vec.size(); i++) {
      Route cur_route;
      Config &cur_cfg = routes_vec[i];
      cur_route.location = cur_cfg["location"].unwrap();
      cur_route.methods =
          Config::split_string(cur_cfg.get_value_safely("methods"));
      cur_route.redirection = cur_cfg.get_value_safely("redirection");
      cur_route.root_dir = cur_cfg.get_value_safely("root_dir");
      cur_route.index = cur_cfg.get_value_safely("index");
      cur_route.error_page = cur_cfg.get_value_safely("error_page");
      cur_route.dir_listing = cur_cfg.get_value_safely("dir_listing") == "true";
      cur_route.default_file_for_dir_request_path =
          cur_cfg.get_value_safely("default_file_for_dir_request_path");
      cur_route.cgi_extension = cur_cfg.get_value_safely("cgi_extension");
      cur_route.upload_files_route =
          cur_cfg.get_value_safely("upload_files_route");
      serv.routes.push_back(cur_route);
    }
    return 0;
  } catch (std::exception &e) {
    logger->log_error("Can't setup one of the servers: " + static_cast<std::string>(e.what()));
    return (-1);
  };
  return 0;
}

int ConnectionManager::start_server(Server &serv) {
  serv.timeout = static_cast<double>(Libft::ft_atoi((*config)["timeout"].unwrap())) / 1000.0;
  serv.cgi_timeout = static_cast<double>(Libft::ft_atoi((*config)["cgi_timeout"].unwrap())) / 1000.0;
  int new_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (new_listen_fd < 0) {
    std::string error = "can't create socket " + serv.host + ":" +
                        Libft::ft_itos(serv.port) + " : " + strerror(errno);
    logger->log_error(error);
    return (-1);
  }
  struct sockaddr_in server_addr_in;
  server_addr_in.sin_family = serv.srv_sockaddr.sin_family;
  server_addr_in.sin_addr.s_addr =
      htonl(serv.srv_sockaddr.sin_addr.s_addr);
  server_addr_in.sin_port = htons(serv.port);
  memset(&(server_addr_in.sin_zero), '\0', 8); // LIBFT bzero
  int opt = 1;
  if (setsockopt(new_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    std::string error = "can't assign address to socket" + serv.host + ":" +
                        Libft::ft_itos(serv.port) + " : " + strerror(errno);
    logger->log_error(error);
    close(new_listen_fd);
    return (-1);
  }
  if (bind(new_listen_fd, (struct sockaddr *)&server_addr_in,
           sizeof(server_addr_in)) < 0) {
    std::string error = "can't bind socket " + serv.host + ":" +
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
  std::string status_message =
      "Server " + serv.host + ":" + Libft::ft_itos(serv.port) + " is listening!";
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
    (*logger).log_error("No servers were successfully set up!");
    return -1;
  }
  listen_servers[fds[0].fd].is_default = true;
  return 0;
}

int ConnectionManager::run() {
  if (listen_servers.empty()) {
    (*logger).log_error("No servers to run! Exitting.");
    return 1;
  }
  while (!sig_stop) {
    int poll_result = poll(fds.data(), fds.size(), 10); // REVISE:timeout from cfg?
    if (poll_result == -1) {
      return handle_poll_error(errno); //REVISE where is terminate()?
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
    (*logger).log_error("Socket " + Libft::ft_itos(fd) + " hung up incorrectly (POLLHUP).");
  } else 
  if (fds[find_fd_index(fd)].revents & POLLNVAL) {
    (*logger).log_error("Socket " + Libft::ft_itos(fd) + " is closed unexpectedly (POLLINVAL).");
    connections[fd].socket_closed = true;
  } else if (fds[find_fd_index(fd)].revents & POLLERR) {
    (*logger).log_error("Socket " + Libft::ft_itos(fd) + " failed (POLLERR).");
  }
  // fds[find_fd_index(fd)].revents = 0;
  close_connection(fd);
}

// when recv error don't need to kill cgi
// because re don't recv if it is running
// can I recv() while CGI is running? I think NO.
bool ConnectionManager::handle_poll_read(int fd) {
  fds[find_fd_index(fd)].revents = 0;
  // fds[find_fd_index(fd)].revents = static_cast<short>(0);
  // (*logger).log_info("poll() read on socket " + Libft::ft_itos(fd));
  HttpConnection &connection = connections[fd];
  if (connection.is_cgi_running) {
    if (cgi_timed_out(fd)) {
      timeout_cgi(fd);
    }
    return false;
  }
  if (connection.recv_done == true) {
    return false;
  }
  (*logger).log_error("---------------------------------" + Libft::ft_itos(fd));
  if (listen_servers.find(fd) != listen_servers.end()) {
    (*logger).log_info("poll() read on SERVER socket " + Libft::ft_itos(fd));
    handle_accept(fd);
    return true;
  }
  // there's data to read from CGI
  if (pipe_to_socket.find(fd) != pipe_to_socket.end()) {
    return handle_cgi_output(connections[pipe_to_socket[fd]]);
  }
  connection.busy = true;
  (*logger).log_info("recv on socket " + Libft::ft_itos(fd));
  char bufg[4001];
  int bytes_recvd = recv(fd, bufg, 4000, 0);
  if (bytes_recvd == 0) {
    // close if not keep-alive!
    if (connection.is_keep_alive == false) {
    // if (true) {
      logger->log_info("socket " + Libft::ft_itos(fd) + " hung up gracefully.");
      close_connection(fd);
      return true;
    }
  }
  connection.update_last_activity();
  if (bytes_recvd < 0) {
    logger->log_error("recv failed on socket " + Libft::ft_itos(fd) +
                      ", closing the connection.");
    exit(1);
    close_connection(fd);
    return true;
  }
  if (bytes_recvd < 4000) {
    connection.recv_done = true;
    fds[find_fd_index(fd)].events |= POLLOUT;
    // fds[find_fd_index(fd)].events
  }
  logger->log_info("Recieved " + Libft::ft_itos(bytes_recvd) + " bytes on socket " +
                   Libft::ft_itos(fd));
  // logger->log_info(bufg); // cout
  connection.recv_stream << bufg;
  bzero(bufg, sizeof(bufg));
  // connection.recv_stream << buffer;
  // bzero(buffer, sizeof(buffer));
  std::string response_string = get_responses_string(connection);
  if (response_string.empty() && connection.is_cgi_running == true) {
    struct pollfd new_pollfd_struct = {connection.cgi_pipe[1], POLLIN,
                                       0};
    fds.push_back(new_pollfd_struct);
    pipe_to_socket[connection.cgi_pipe[0]] = fd;
  }
  connection.send_buffer.append(response_string);
  return true;
}

void ConnectionManager::handle_accept(int fd) {
  sockaddr_in socket_address;
  socklen_t socket_address_length = sizeof(socket_address);
  int new_fd = accept(fd, (sockaddr *)&socket_address, &socket_address_length);
  int one = 1;
  if (new_fd == -1) {
    logger->log_error("Failed to accept a new connection on socket" +
                      Libft::ft_itos(fd));
    return;
  }
  setsockopt(new_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
  (*logger).log_info("Accepted new connection " + Libft::ft_itos(new_fd) + " on socket " + Libft::ft_itos(fd));

  struct pollfd new_pollfd_struct;
  new_pollfd_struct.fd = new_fd;
  new_pollfd_struct.events = POLLIN;
  // fds[find_fd_index(fd)].revents = static_cast<short>(0);
  fds.push_back(new_pollfd_struct);
  HttpConnection connection(config, &listen_servers[fd]);
  connection.fd = new_fd;
  connections[new_fd] = connection;
  connection.update_last_activity();
}

bool ConnectionManager::handle_poll_write(int fd) {
  fds[find_fd_index(fd)].revents = 0;
  // fds[find_fd_index(fd)].revents = static_cast<short>(0);
  if (conn_timed_out(fd)) {
    close_connection(fd);
    return false;
  }
  if (connections[fd].is_cgi_running) {
    if (cgi_timed_out(fd)) {
      timeout_cgi(fd);
    }
    return false;
    // return handle_cgi_output(connections[fd]); // - removed. Can 
    // not do it because we didn't check if poll() said we even can read() form the pipe!
  }

  if (connections[fd].send_buffer.empty()) {
    return false;
  }
  int bytes_sent = send(fd, connections[fd].send_buffer.c_str(),
                        connections[fd].send_buffer.length(), 0);
  connections[fd].update_last_activity();
  if (bytes_sent < 0) {
    logger->log_error("send failed on socket" + Libft::ft_itos(fd));
    close_connection(fd);
    return true;
  }
  connections[fd].send_buffer.erase(0, bytes_sent);
  if (connections[fd].send_buffer.empty()) {
    fds[find_fd_index(fd)].events = POLLIN;
    connections[fd].recv_done = false;
  }
  return true;
}

bool ConnectionManager::handle_cgi_output(HttpConnection &connection) {
  if (!connection.cgi_finished) {
    int status_code;
    connection.cgi_result = waitpid(connection.cgi_pid, &status_code, WNOHANG);
    if (connection.cgi_result == 0) {
      return false; // NB cgi is still running
    }
    if (connection.cgi_result != connection.cgi_pid) {
      // NB unknown waitpid error - strange pid returned
      kill_cgi(connection.fd);
      logger->log_error("waitpid returned garbage pid on socket " +
                        Libft::ft_itos(connection.fd));
      return false;
    }
    if (!WIFEXITED(status_code)) {
      kill_cgi(connection.fd);
      connection.send_buffer = "CGI ERROR";
      return false;
    }
    connection.cgi_finished = true;
  }
  return read_cgi_pipe(connection);
}

//assumes poll() said we can read from the pipe!
bool ConnectionManager::read_cgi_pipe(HttpConnection &connection) {
  int buf_len = sizeof(cgi_buffer);
  (*logger).log_info("reading CGI" + Libft::ft_itos(connection.cgi_pipe[0]) + "on socket" + \
                     Libft::ft_itos(connection.fd));
  bzero(cgi_buffer, buf_len);
  int bytes_read = read(connection.cgi_pipe[0], cgi_buffer, buf_len);
  connection.update_last_cgi_activity();
  if (bytes_read < 0) {
    logger->log_error("read() failed on CGI pipe for socket " +
                      Libft::ft_itos(connection.fd));
    connection.send_buffer = "INTERNAL SERVER ERROR";
    connection.is_response_ready = true;
    connection.busy = false;
    kill_cgi(connection.fd);
    return true;
  }
  connection.send_buffer.append(cgi_buffer);
  bzero(cgi_buffer, buf_len);
  if (bytes_read < buf_len) {
    kill_cgi(connection.fd);
    connection.cgi_finished = true;
    connection.is_response_ready = true;
    connection.busy = false;
  }
  return true;
}

/*------------ UTILS ----------------*/

/**
 * @brief takes connection and correctly closes it, closes the sockets and pipes etc
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
  for (std::vector<struct pollfd>::iterator it = fds.begin();
       it != fds.end();it++) {
    if (it->fd == fd) {
      fds.erase(it);
      break;
    }
  }
  connections.erase(fd);
  (*logger).log_info("Closed the connection on socket " + Libft::ft_itos(fd));
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
}

void ConnectionManager::timeout_cgi(int connection_fd) {
  kill_cgi(connection_fd);
  connections[connection_fd].send_buffer = "CGI TIMEOUT";
  connections[connection_fd].is_cgi_running = false;
  connections[connection_fd].is_response_ready = true;
}


void ConnectionManager::shutdown_server(int listen_fd) {
  Server *srv = &(listen_servers[listen_fd]);
  // FIX crashes idk, maybe create a copy of connections so we don't modify what is inside for parantheses?
  for (std::map<int, HttpConnection>::iterator it = connections.begin();
        it != connections.end();) {
    if (it->second.serv == srv) {
      int fd_to_remove = it->second.fd;
      it++;
      close_connection(fd_to_remove);
      continue;
    }
    it++;
  }
  close(srv->listen_fd);
  for (std::vector<struct pollfd>::iterator it = fds.begin();
       it != fds.end();it++) {
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
    shutdown_server(it->first);
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
    logger->log_error("poll() error - EFAULT");
    return -1;
  } else if (err_num == EINVAL) {
    logger->log_error("poll() error - EFAULT");
    return -1;
  } else if (err_num == ENOMEM) {
    logger->log_error("poll() error - EFAULT");
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
  throw std::out_of_range("The pollfd struct with the sought-after ->fd is not found");
  return 0;
}



#define DEBUG
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
       it != connections.end(); it++) {
    std::cout << "\t" << it->first << ": ";
    it->second.print_connection();
    // std::cout << std::endl;
  }
  std::cout << std::endl;

  std::cout << "listen_servers list:" << std::endl;
  for (std::map<int, Server>::iterator it = listen_servers.begin();
       it != listen_servers.end(); it++) {
    std::cout << "\t{" << it->first << ": \n";
    it->second.print_server();
  }
  std::cout << "}" << std::endl;

  std::cout << "pipe_to_socket list:" << std::endl;
  for (std::map<int, int>::iterator it = pipe_to_socket.begin();
       it != pipe_to_socket.end(); it++) {
    std::cout << "\t" << it->first << ": ";
    std::cout << it->second;
    std::cout << std::endl;
  }
  std::cout << std::endl;
  std::cout << "====\n====\n====\n";
}
#endif