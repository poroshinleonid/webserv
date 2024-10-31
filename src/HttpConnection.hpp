#ifndef HTTPCONNECTION_HPP
#define HTTPCONNECTION_HPP

#include "Config.hpp"
#include "Logger.hpp"
#include "Server.hpp"

#include <iostream>
#include <map>
#include <sstream>
#include <string>
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

enum class HttpChunkState {
  DATA_CHUNK,
  END_CHUNK,
  NOT_A_CHUNK,
};

struct HttpChunk {
  HttpChunkState state;
  std::string content;
};

class HttpConnection {
public:
  HttpConnection();
  ~HttpConnection();
  HttpConnection(HttpConnection const &other);
  HttpConnection &operator=(const HttpConnection &obj);

public:
  HttpConnection(Config *cfg, Logger *log, Server *srv);

public:
  void update_last_activity();
  void update_last_cgi_activity();
  HttpChunk parse_http_chunk();

// Add a buffer for extra requests (in case we have a string with several requests)
// and we eat only one, the others should be saved
// This value will have to be checked and cleaned at several important places inside the ConnManager
public:
  int fd;
  std::string recv_chunk;
  Config *config;
  Logger *logger;
  Server *serv;
  std::string recv_buffer;
  std::string send_buffer;
  std::string cgi_write_buffer;
  HttpChunk chunked_chunk;
  std::string next_requests_str;
  time_t last_activity;
  time_t last_cgi_activity;
  int content_length;
  bool is_connected;
  bool is_cgi_running;
  std::string cgi_response;
  bool cgi_finished;
  pid_t cgi_result;
  bool socket_closed;
  bool is_response_ready;
  bool is_keep_alive;
  bool close_after_send;
  bool reading_garbage_chunks;
  pid_t cgi_pid;
  int cgi_pipe[2];
  bool is_chunked_transfer;
  bool recv_done;

#define DEBUG
#ifdef DEBUG
public:
  void print_connection();
#endif
};

#endif // HTTPCONNECTION_HPP