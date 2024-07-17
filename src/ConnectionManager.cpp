#include "Config.hpp"
#include "ConnectionManager.hpp"
#include "HttpConnection.hpp"


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

ConnectionManager::ConnectionManager(Config &cfg) : config(cfg) {
  // config = cfg;
  bzero(buffer, sizeof(buffer));
  bzero(cgi_buffer, sizeof(cgi_buffer));
}

ConnectionManager::~ConnectionManager() {
  // free(buffer);
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

// fix handle_ functions return values
int ConnectionManager::handle_fds(int fd_count) {
  (void)fd_count;
  int bytes_handled;
  for (int i = 0, sz = fds.size(); i < sz; i++) {
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
  // if taaae_fun() runs cgi it changes is_forked,cgi_pid,cgi_pipe[] vars
  // returns string to_send
  // if empty string the I do noghing send nogthibg maybe check is_cgi_running
  // std::string get_responses_string(HttpConnection &connection);

  // if (connection.is_chunked_transfer) {

  // }
  // connection.recv_buffer.append(buffer);
  // bzero(buffer, sizeof(buffer));
  // if (!connection.header_is_parsed) {
  //   size_t header_end_pos;
  //   header_end_pos = connection.recv_buffer.find("\r\n\r\n");
  //   if (header_end_pos != std::string::npos) {
  //     connection.header_str = connection.recv_buffer.substr(0, header_end_pos + 4);
  //     //FIX FIND CONTENT-LENGTH AND SET IT
  //     //FIX handle chunk send method
  //     connection.recv_buffer.erase(0, header_end_pos + 4);
  //     connection.header_is_parsed = true;
  //   }
  // }
  // //now if header is parsed then parse the body
  // //if header is parsed and the length of content equals request's content length, set body_is_read = true;
  // if (connection.header_is_parsed && connection.recv_buffer.length() >= connection.content_length) {
  //   connection.body_str = connection.recv_buffer.substr(0, connection.content_length);
  //   connection.recv_buffer.erase(0, connection.content_length);
  //   connection.body_is_read = true;
  // }

  // if (connection.body_is_read == true) {
  //   // std::string get_response_string(const std::string &request_string);
  //   connection.send_buffer.append(get_response_string(connection));
  //   process_request(connection);
  // }
  // return bytes_recvd;
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
  fds[fd].events |= POLLIN;
  std::cout << "WRITING" << std::endl; // FIX

}

int ConnectionManager::handle_accept(int fd) {
	sockaddr_in socket_address;
	socklen_t socket_address_length = sizeof(socket_address);
  int new_fd = accept(fd, (sockaddr *)&socket_address, &socket_address_length);
  if (new_fd == -1) {
    return (-1);
  }
  HttpConnection connection(config);
  // FIX do it in the constructor
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
  new_pollfd_struct.events = 0;
  new_pollfd_struct.revents = 0;
  fds.push_back(new_pollfd_struct);
  connections[new_fd] = connection;
  return new_fd;

}


int ConnectionManager::process_request(HttpConnection &connection) {
  return 0;
  std::cout << "process_request called!" << std::endl;
  HttpRequest request(connection.recv_buffer);
  // FIX - this constructor should fill in the request
  //check request validity if request.isValid()
  if (request.is_for_cgi) {
    return run_cgi(connection, request);
  }
  if (connection.cgi_finished) {
    answer_request_with_string(connection);
  } else {
    answer_request(connection, request);
  }
}

int ConnectionManager::answer_request(HttpConnection &connection, HttpRequest &request) {
  //forms the writebuffer
  //calls answer_request_with_string to send the said buffer
  //if al sent, reset the state, depending on keep-alive and stuff like that
}

int ConnectionManager::answer_request_with_string(HttpConnection &connection) {
  //send the cgi_string to the connection
}


int ConnectionManager::run_cgi(HttpConnection &connection, HttpRequest &request) {
  if (connection.is_cgi_running) {
    return exec_cgi(connection, request);
  }
  std::string cgi_response_tmp = try_read_fork(connection, request);
  if (cgi_response_tmp == "error of some type") { // FIX: error codenames, etc
    connection.cgi_response = cgi_response_tmp;
    return -1;
  } else if (cgi_response_tmp == "fin") {
    connection.send_buffer = connection.cgi_response;
    connection.cgi_response.clear();
    connection.cgi_finished = true;
    return 0;
  }
  connection.cgi_response.append(cgi_response_tmp);
  return 0;
}

int ConnectionManager::exec_cgi(HttpConnection &connection, HttpRequest &request) {
  pid_t pid = fork();
  if (pid == -1) {
    return -1;
  }
  if (pid != 0) {
    connection.cgi_pid = pid;
    connection.is_cgi_running = true;
    connection.cgi_finished = false;
    connection.busy = true;
    return pid;
  }
  // FIX form pipes
  // FIX actually send something to the script
  execve("./test/cgi/cgi1.py", NULL, NULL); // FIX cgi path should be stored in config, should it?
  //4. if child, do prep and send all the necessary data to execve
  exit(-1);
}

std::string ConnectionManager::try_read_fork(HttpConnection &connection, HttpRequest &request) {
  std::string s;
  //  1. if fork timed out, die
  //  2. do waitpid() to check the status of the process
  /*  3. if fork error then handle it
      if fork WIFEXITED sucessfully, read all of the data in a while loop  to a string (FIX: add hasExited variable to save this and make this string a part of Connection instance)!
      ofc handle all errors
      Return the read string*/
  return s;
}



void ConnectionManager::update_last_activity(HttpConnection &connection) {
  time_t new_time;
  new_time = std::time(&new_time);
  if (new_time == -1) {
    // FIX - log an error, maybe throw an exception and kill the whole connection of the whole server idk
    return;
  }
  connection.last_activity = new_time;
  return;
}
