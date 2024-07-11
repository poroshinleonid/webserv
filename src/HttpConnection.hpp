#ifndef HTTPCONNECTION_HPP
#define HTTPCONNECTION_HPP

#include "Config.hpp"
#include "ServerConfig.hpp"

#include <string>
#include <map>
#include <iostream>
#include <vector>

enum connection_state {
  CON_WAIT,
  CON_CGI_RUNNING,
  CON_CGI_FIN,
  CON_CGI_TIMEOUT,
  CON_CGI_ERR,
  CON_RECV_INCOMPL,
  CON_RECV_COMPL,
  CON_SEND_INCOMPL,
  CON_ERR,
  CON_CLOSED,
};

class	HttpConnection {
public:
  HttpConnection();
  HttpConnection(HttpConnection const &other);
  ~HttpConnection();
  HttpConnection &operator=(const HttpConnection &obj);

public:
  HttpConnection(Config &cfg);
  HttpConnection(int fd, ServerConfig &scfg, Config &cfg);


public:
  int fd;
  int port;
  std::string host;
  Config &config;
  std::string recv_buffer;
  std::string send_buffer;
  time_t last_activity;
  bool busy;
  bool is_connected;
  bool is_cgi_running;
  std::string cgi_response;
  bool cgi_finished;
  bool should_die;
  bool is_response_ready;
  bool is_keep_alive;
  bool header_is_parsed;
  bool body_is_read;
  pid_t cgi_pid;
  int cgi_pipe[2];
};

#endif // HTTPCONNECTION_HPP
