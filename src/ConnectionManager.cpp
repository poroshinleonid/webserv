#include "Config.hpp"
#include "ConnectionManager.hpp"
#include "HttpConnection.hpp"
#include "Server.hpp"


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
#include <stdlib.h>

//FIX
#define CGI_TIMEOUT 100000

// std::string unwrap(); // returns content string or throws std::invalid_argument
// std::string get_content();
// Config operator[](const std::string& key); // gets config by key, throws std::out_of_range if not found, if multiple elements, returns the last one
// vector<Config> get_vec(const std::string& key); // returns vector of configs (one entry for each key entry)

ConnectionManager::ConnectionManager(Config &cfg, Logger &log) : config(cfg), logger(log) {
  bzero(buffer, sizeof(buffer));
  bzero(cgi_buffer, sizeof(cgi_buffer));
}

ConnectionManager::~ConnectionManager() {}


// FIX
int ft_atoi(const std::string &s) {
  return std::stoi(s);
}
//FIX
int ft_atoip(const std::string &s) {

  int result;
  inet_pton(AF_INET, s.c_str(), &result);
  return result;
}

int add_listen_server(Config &cfg) {
  Server serv;
  serv.port = ft_atoi(cfg["port"].unwrap());
  serv.host =cfg["host"].unwrap();
  // server_name
  // default error pages
  // client_body_size (??)
  // FIX somehow parse all of the routes
  // for (int i = 0; i < cfg["routes"].length(); i++) {
  //   routes.append()
  // }

  // FIX - handle errors!
  /* ----- Listening socket init*/
  int new_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (new_listen_fd < 0) {
      perror("socket");
    return (-1);
  }
  struct sockaddr_in server_addr_in;
  server_addr_in.sin_family = AF_INET;
  server_addr_in.sin_addr.s_addr = htons(serv.host); 
  server_addr_in.sin_port = htons(serv.port);
  memset(&(server_addr_in.sin_zero), '\0', 8);
  int opt = 1;
  // FIX - handle errors!
  if (setsockopt(new_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt");
    return (-1);
  }
  // FIX - handle errors!
  if (bind(new_listen_fd, (struct sockaddr *)&server_addr_in, sizeof(server_addr_in)) < 0) {
      perror("bind");
      return (-1);
  }
  std::cout << "I HAVE BINDED THE SERVER\n";
  // FIX - handle errors!
  // FIX - load data about listen backlog from the config or from somewhere!
  if (listen(new_listen_fd, 20) == -1)
  {
    perror("listen");
    return (-1);
  }

  // listen_fds.push_back(new_listen_fd);
  serv.listen_fd = new_listen_fd;
  listen_servers[serv.listen_fd] = serv;
  struct pollfd poll_fd = {serv.listen_fd, POLLIN, 0};
  fds.push_back(poll_fd);
  return 0;
}


int ConnectionManager::setup() {
  std::vector<Config> server_configs;
  server_configs = cfg.get_vec("server");
  for (int i = 0; i < server_configs.length(); i++) {
    try {
      add_listen_server(server_configs[i]);
    } catch (std:exception &e) {
      std::cerr << e.what() << std::endl;
    }
  }
  //FIX log and exit
  if (listen_servers.empty()) {
    std::cout << "no servers loaded successfully, nothing to do, bye" << std::endl;
    return -1;
  }
  listen_servers[listen_fds[0]].is_default = true;

}

int ConnectionManager::run() {
  std::cout << "Starting poll() loop" << std::endl;
  // FIX: timeout to poll is probly taken from the config
  while (true) {
    // FIX: set all poll revents to zero!
    int poll_result = poll(fds.data(), fds.size(), 10);
    if (poll_result == -1) {
      std::cout << "poll error" << std::endl; // FIX log
      exit(1);
    }
    if (poll_result == 0) {
      handle_timeouts();
      continue;
    }
    handle_fds(poll_result);
  }
  return 0;
}


void ConnectionManager::update_last_activity(HttpConnection &connection) {
  time_t new_time;
  new_time = std::time(&new_time);
  if (new_time == -1) {
    return;
  }
  connection.last_activity = new_time;
  return;
}


int ConnectionManager::handle_timeouts(int fd) {
  //FIX forbidden function
  HttpConnection &connection = connections[fd];
  time_t cur_time;
  time(cur_time);
  for (int i = 0; i < connection.length(); i++) {
    if (connection.is_cgi_running && 
       (cur_time - connection.last_cgi_activity) > CGI_TIMEOUT) {
      //timeout only CGI
      ;
    }
    if ((cur_time - connection.last_activity) > CONN_TIMEOUT)
      //timeout the connection
      ;
  }
}

// fix handle_ functions return values
int ConnectionManager::handle_fds() {
  bool io_happened = false;
  int bytes_handled;
  for (int i = 0, sz = fds.size(); i < sz; i++) {
    if (io_happened) {
      check_timeouts(i);
      continue;
    }
    if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
      handle_poll_problem(i);
      break;
    } else if (fds[i].revents & POLLIN) {
      bytes_handled = handle_poll_read(i);
      break;
    } else if (fds[i].revents & POLLOUT) {
      handle_poll_write(i);
      break;
    }
    //handle_timeouts() ?? // cleanup() also kills connections with should_die=true
  }
}

int ConnectionManager::handle_poll_problem(int fd) {
  // if listen_fd is dead, restart "server" of just kill it and say it died?
  std::cout << "POLL ERROR" << std::endl; // FIX
}

// make recv buffer for only one request
int ConnectionManager::handle_poll_read(int fd) {
  //FIX m.find("f") == m.end()
  //map instead of vec!
  if (std::find(listen_fds.begin(), listen_fds.end(), fd) == listen_fds.end()) {
    return handle_accept(fd);
  }
  HttpConnection &connection = connections[fd];
  connection.busy = true;
  int bytes_recvd = recv(fd, buffer, sizeof(buffer), 0);
  if (bytes_recvd < 0) {
    //LOG log the error
    close(fd);
    connections.erase(fd);
    return -1;
  }
  if (bytes_recvd == 0) {
    //LOG log that the client said goodbye
    close(fd);
    connections.erase(fd);
    return 0;
  }
  connection.recv_stream << buffer;
  bzero(buffer, sizeof(buffer));
  std::string response_string = get_responses_string(connection);
  if (response_string.empty()) {
    //there's no response for whatever reason
    return bytes_recvd;
  }
  connection.response_string.append(response_string);
  return bytes_recvd;
}

int ConnectionManager::handle_poll_write(int fd) {
  if (connections[fd].is_cgi_running) {
    /*if cgi is running, try getting the input from it */
    /* if all read, cgi is gonna die (or do I kill it) and set is_cgi_running = false*/
    return 0;
  }

  /*
  if nothing to send, remove "WRITE" from poll fd events\
  send the biggest of (default_send_chunk_size, send_buffer.lengt())
  handle different bytes_read
  */
  // fds[fd].events |= POLLIN;
  std::cout << "WRITING" << std::endl; // FIX
  return 0;
}

int ConnectionManager::handle_accept(int fd) {
  //FIX: add fd to poll() list, set .events flags
	sockaddr_in socket_address;
	socklen_t socket_address_length = sizeof(socket_address);
  int new_fd = accept(fd, (sockaddr *)&socket_address, &socket_address_length);
  if (new_fd == -1) {
    return (-1);
  }
  HttpConnection connection(config); //FIX - serverconf instead of general conf
  connection.fd = new_fd;
  connection.port = (int)socket_address.sin_port;
  connection.config = config;
  connection.busy = false;
  connection.is_connected = true;
  connection.is_cgi_running = false;
  connection.cgi_finished = false;
  connection.should_die = false;
  connection.is_response_ready = false;
  connection.is_keep_alive = false;
  connection.header_is_parsed = false;
  connection.body_is_read = false;
  connection.cgi_pid = 0;
  connection.cgi_pipe[0] = 0;
  connection.cgi_pipe[1] = 0;
  struct pollfd new_pollfd_struct;
  new_pollfd_struct.fd = new_fd;
  new_pollfd_struct.events = POLLIN | POLLOUT;
  new_pollfd_struct.revents = 0;
  fds.push_back(new_pollfd_struct);
  connections[new_fd] = connection;
  return new_fd;

}