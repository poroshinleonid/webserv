#ifndef HTTPCONNECTION_HPP
#define HTTPCONNECTION_HPP

#include "Config.hpp"

#include <string>
#include <map>
#include <iostream>
#include <vector>

class	HttpConnection {
private:
  HttpConnection();
  HttpConnection(HttpConnection const &other);
  ~HttpConnection();
  HttpConnection &operator=(const HttpConnection &obj);

public:
  int fd;
  int port;
  Config &config;
  std::string recv_buffer;
  std::string send_buffer;
  time_t last_activity;
  bool is_connected;
  bool has_forked;
  bool cgi_finished;
  bool should_die;
  bool is_response_ready;
  bool is_keep_alive;
  bool header_is_read;
  bool body_is_read;
  pid_t child_pid;
  int cgi_pipe[2];

};

#endif // HTTPCONNECTION_HPP
