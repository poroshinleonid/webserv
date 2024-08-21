#include "Config.hpp"
#include "ConnectionManager.hpp"
#include "HttpConnection.hpp"
#include "Server.hpp"

#include <iostream>
#include <map>
#include <vector>
#include <cstring>
#include <algorithm>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <ctime>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>

std::string get_responses_string(HttpConnection &connection) {
  (void)connection;
  return "HTTP/1.1 200 OK\r\n"
                                              "Content-Type: text/plain\r\n"
                                              "Content-Length: 12\r\n"
                                              "\r\n"
                                              "Hello world!";
}

//FIX: is std::time even allowed?
//FIX
#define CGI_TIMEOUT 100000
#define CONN_TIMEOUT 100000

ConnectionManager::ConnectionManager(Config *cfg, Logger *log) : config(cfg), logger(log) {
  bzero(buffer, sizeof(buffer));
  bzero(cgi_buffer, sizeof(cgi_buffer));
}

ConnectionManager::~ConnectionManager() {
  for (size_t i = 0; i < fds.size(); i++) {
    close(i);
  }
}


int ft_atoi(const std::string &s) {
// FIX
  return std::atoi(s.c_str());
}

int ft_atoip(const std::string &s) {
//FIX

  int result;
  inet_pton(AF_INET, s.c_str(), &result);
  return result;
}

std::string ft_itos(int number) {
  std::ostringstream strm;
  std::string s;
  strm << number;
  s = strm.str();
  return s;
}

int ConnectionManager::setup_server(Server &serv, Config &cfg) {

  serv.port = ft_atoi(cfg["port"].unwrap());
  serv.host = cfg["host"].unwrap();
  serv.host_struct.s_addr = Config::string_to_ip(serv.host);
  serv.timeout = 1000; // ft_atoi(cfg["timeout"].unwrap());
  // server_name
  // default error pages
  // limit client_body_size (??)
  // FIX somehow parse all of the routes
  // for (int i = 0; i < cfg["routes"].length(); i++) {
  //   routes.append()
  // }
  return 0;
}

int ConnectionManager::start_server(Server &serv) {
  int new_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (new_listen_fd < 0) {
    std::string error = "can't create socket " + serv.host + ":" + ft_itos(serv.port) + " : " + strerror(errno);
    logger->log_error(error);
    return (-1);
  }
  struct sockaddr_in server_addr_in;
  server_addr_in.sin_family = AF_INET;
  server_addr_in.sin_addr.s_addr = htonl(serv.host_struct.s_addr); //FIX htonl(serv.host_struct); 
  server_addr_in.sin_port = htons(serv.port);
  memset(&(server_addr_in.sin_zero), '\0', 8);
  int opt = 1;
  if (setsockopt(new_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    std::string error = "can't assign address to socket" + serv.host + ":" + ft_itos(serv.port) + " : " + strerror(errno);
    logger->log_error(error);
    close(new_listen_fd);
    return (-1);
  }
  if (bind(new_listen_fd, (struct sockaddr *)&server_addr_in, sizeof(server_addr_in)) < 0) {
    std::string error = "can't bind socket " + serv.host + ":" + ft_itos(serv.port) + " : " + strerror(errno);
    logger->log_error(error);
    close(new_listen_fd);
      return (-1);
  }
  // FIX - load data about listen backlog from the config or from somewhere!
  if (listen(new_listen_fd, 20) == -1)
  {
    std::string error = "can't listen to socket" + serv.host + ":" + ft_itos(serv.port) + " : " + strerror(errno);
    logger->log_error(error);
    close(new_listen_fd);
    return (-1);
  }
  std::string status_message = "Server " + serv.host + ":" + ft_itos(serv.port) + " is listening!";
  logger->log_info(status_message);
  serv.listen_fd = new_listen_fd;
  listen_servers[serv.listen_fd] = serv;
  struct pollfd poll_fd = {serv.listen_fd, POLLIN | POLLOUT, 0};
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


int ConnectionManager::setup() {
  std::vector<Config> server_configs;
  server_configs = config->get_vec("server");
  for (size_t i = 0; i < server_configs.size(); i++) {
    add_listen_server(server_configs[i]);
  }
  //FIX log and exit
  if (listen_servers.empty()) {
    std::cout << "no servers loaded successfully, nothing to do, bye" << std::endl;
    return -1;
  }
  listen_servers[fds[0].fd].is_default = true;
  return 0;
}

int ConnectionManager::run() {
  while (true) {
    int poll_result = poll(fds.data(), fds.size(), 10); //FIX:timeout from cfg?
    if (poll_result == -1) {
      logger->log_error("fatal error in poll()");
      return -1;
    }
    if (poll_result == 0) {
      continue;
    }
    handle_fds();
  }
  return 0;
}

// FIX
int ConnectionManager::cleanup(int fd) {
  (void)fd;
  //FIX forbidden function
  // HttpConnection &connection = connections[fd];
  // time_t cur_time;
  // cur_time = std::time(&cur_time);
  // for (size_t i = 0; i < connection.length(); i++) {
  //   if (connection.is_cgi_running && 
  //      (cur_time - connection.last_cgi_activity) > CGI_TIMEOUT) {
  //     //timeout only CGI
  //     ;
  //   }
  //   if ((cur_time - connection.last_activity) > CONN_TIMEOUT)
  //     //timeout the connection
  //     ;
  // }
  // //check other errors
  return 0;
}

// fix handle_ functions return values
int ConnectionManager::handle_fds() {
  bool io_happened = false;
  for (int i = 0, sz = fds.size(); i < sz; i++) {
    int fd = fds[i].fd;
    if (io_happened) {
      cleanup(fd);
    } else if (fds[i].revents & (POLLERR | POLLNVAL)) {
      io_happened = handle_poll_problem(fd);
    } else if (fds[i].revents & POLLIN) {
      io_happened = handle_poll_read(fd);
    } else if (fds[i].revents & POLLOUT) {
      io_happened = handle_poll_write(fd);
    }
  }
  return 0;
}

bool ConnectionManager::handle_poll_problem(int fd) {

  (void)fd;
  //FIX kill serv if err, webserv if fatal, nothing if small err
  logger->log_error("some poll error");
  fds[fd].revents = 0;
  return false;
}

// FIX add cgi handling when dying
// FIX also remove fd when dying
bool ConnectionManager::handle_poll_read(int fd) {
  fds[fd].revents = 0;
  if (connections[fd].recv_done == true) {
    return false;
  }
  if (listen_servers.find(fd) != listen_servers.end()) {
    handle_accept(fd);
    return true;
  }
  if (pipe_to_socket.find(fd) != pipe_to_socket.end()) {
    return handle_cgi_output(connections[fd]);
  }
  HttpConnection &connection = connections[fd];
  connection.busy = true;
  std::cout << "recv socket " << fd << std::endl;
  char bufg[4001];
  int bytes_recvd = recv(fd, bufg, 4000, 0);
  // int bytes_recvd = recv(fd, buffer, sizeof(buffer), 0);
  if (bytes_recvd < 0) {
    logger->log_error("recv failed on socket " + ft_itos(fd) + ", closing the connection.");
    exit(1);
    close_connection(fd);
    return true;
  }
  if (bytes_recvd == 0) {
    logger->log_info("socket " + ft_itos(fd) + "hung up.");
    // close if not keep-alive!
    close_connection(fd);
    return true;
  }
  if (bytes_recvd < 4000) {
    connections[fd].recv_done = true;
  }
  logger->log_info("Recieved " + ft_itos(bytes_recvd) + " bytes on socket " + ft_itos(fd));
  connection.recv_stream << bufg;
  bzero(bufg, sizeof(bufg));
  // connection.recv_stream << buffer;
  // bzero(buffer, sizeof(buffer));
  std::string response_string = get_responses_string(connection);
  if (response_string.empty() && connection.is_cgi_running == true) {
    struct pollfd new_pollfd_struct = {connection.cgi_pipe[1],POLLIN | POLLOUT,0};
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
  if (new_fd == -1) {
    logger->log_error("Failed to accept a new connection on socket" + ft_itos(fd));
    return;
  }

  
  std::cout << "ACCEPTED SOCKET " << fd <<", new one is " << new_fd << std::endl;

  // FIX - crasheson thisparagraph
  struct pollfd new_pollfd_struct;
  new_pollfd_struct.fd = new_fd;
  new_pollfd_struct.events = POLLIN | POLLOUT;
  new_pollfd_struct.revents = 0;
  fds.push_back(new_pollfd_struct);
  // write(STDOUT_FILENO, "A", 1);
  // printf("SDFSFSDF");
  // std::cout << "EEEEEE";
  HttpConnection connection(config, &listen_servers[fd]);
  connection.fd = new_fd;
  connection.update_last_activity();
  connections[new_fd] = connection;
  fds[fd].revents = 0;

}




bool ConnectionManager::handle_poll_write(int fd) {
  fds[fd].revents = 0;
  if (conn_timed_out(fd)) {
    close_connection(fd);
    return false;
  }
  if (connections[fd].is_cgi_running) {
    if (cgi_timed_out(fd)) {
      kill_cgi(fd);
      connections[fd].send_buffer = "CGI TIMEOUT";
      return false;
    }
    return handle_cgi_output(connections[fd]); //NB will read() from pipe
  }

  if (connections[fd].send_buffer.empty()) {
    return false;
  }

  int bytes_sent = send(fd, connections[fd].send_buffer.c_str(), connections[fd].send_buffer.length(), 0);
  if (bytes_sent < 0) {
    logger->log_error("send failed on socket" + ft_itos(fd));
    close_connection(fd);
    return true;
  }
  connections[fd].send_buffer.erase(0, bytes_sent);
  if (connections[fd].send_buffer.empty()) {
    connections[fd].recv_done = false;
  }
  connections[fd].update_last_activity();
  return true;
}

bool ConnectionManager::handle_cgi_output(HttpConnection &connection) {
  if (!connection.cgi_finished) {
    int status_code;
    connection.cgi_result = waitpid(connection.cgi_pid, &status_code, WNOHANG);
    if (connection.cgi_result == 0) {
      //NB cgi is still running
      return false;
    }
    if (connection.cgi_result != connection.cgi_pid) {
      //NB unknown waitpid error - strange pid returned
      kill_cgi(connection.fd);
      logger->log_error("waitpid returned garbage pid on socket " + ft_itos(connection.fd));
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

bool ConnectionManager::read_cgi_pipe(HttpConnection &connection) {
  connection.update_last_activity();

  int buf_len = sizeof(cgi_buffer);
  std::cout << "cgi read\n";
  int bytes_read = read(connection.cgi_pipe[0], cgi_buffer, buf_len);
  if (bytes_read < 0) {
    logger->log_error("read() failed on CGI pipe for socket " + ft_itos(connection.fd));
    connection.send_buffer = "INTERNAL SERVER ERROR";
    kill_cgi(connection.fd);
    return true;
  }
  connection.send_buffer.append(cgi_buffer);
  bzero(cgi_buffer, buf_len);
  if (bytes_read == buf_len) {
    return true;
  }
  kill_cgi(connection.fd);
  connection.cgi_finished = true;
  return true;
}









/*------------ UTILS ----------------*/

void ConnectionManager::close_connection(int fd) {
  if (connections[fd].is_cgi_running) {
    kill_cgi(fd);
  }

  // fds.erase(fds.find(fd));std::find_if
  for (std::vector<struct pollfd>::iterator it = fds.begin(); it != fds.end();) {
    if (it->fd == fd) {
      fds.erase(it);
      break;
    }
    it++;
  }
  
  connections.erase(fd);
}


bool ConnectionManager::conn_timed_out(int fd) {
  time_t now;
  now = std::time(&now);
  double connection_age = std::difftime(connections[fd].last_activity,  now);
  if (connection_age > CONN_TIMEOUT) { // FIX - get from config
    return true;
  }  
  return false;
}

bool ConnectionManager::cgi_timed_out(int fd) {
  time_t now;
  now = std::time(&now);
  double cgi_age = std::difftime(connections[fd].last_activity,  now);
  if (cgi_age > CONN_TIMEOUT) { // FIX - get from config
    return true;
  }  
  return false;
}

//Do I clear the buffer or not?
// maybe make kill_cgi() and stop_cgi() different funcs?
void ConnectionManager::kill_cgi(int fd) {
  kill(connections[fd].cgi_pid, SIGKILL);
  close(connections[fd].cgi_pipe[0]);
  pipe_to_socket.erase(connections[fd].cgi_pipe[0]);
  connections[fd].is_cgi_running = false;
  //FIX connections[fd].send_buffer.clear();
}