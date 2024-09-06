#include "HttpConnection.hpp"
#include "Server.hpp"

#include <ctime>
#include <unistd.h>

HttpConnection::HttpConnection() {}

HttpConnection::~HttpConnection() {
  // if (cgi_pipe[0] != 0){
  //   close(cgi_pipe[0]);
  // }
  // if (cgi_pipe[1] != 1){
  //   close(cgi_pipe[1]);
  // }
}

HttpConnection::HttpConnection(Config *cfg, Server *srv)
    : config(cfg), serv(srv) {
  (void)cfg;
  (void)srv;
  content_length = -1;
  busy = false;
  is_connected = true;
  is_cgi_running = false;
  cgi_finished = false;
  should_die = false;
  is_response_ready = false;
  is_keep_alive = false;
  header_is_parsed = false;
  body_is_read = false;
  cgi_pid = 0;
  cgi_pipe[0] = 0;
  cgi_pipe[1] = 0;
  is_chunked_transfer = false;
}

// FIX remove?
//  HttpConnection::HttpConnection(int fd, Server *serv, Config *cfg)  :
//      fd(fd), config(cfg), serv(serv),
//      recv_buffer(""), send_buffer(""),
//      header_str(""), body_str(""), content_length(0),
//      busy(false), is_connected(true),
//      is_cgi_running(false),
//      cgi_response(""),
//      cgi_finished(false), should_die(false),
//      is_response_ready(false),
//      is_keep_alive(false), header_is_parsed(false),
//      body_is_read(false), cgi_pid(0) {
//      last_activity = std::time(&last_activity);
//      (void)fd;
//      (void)serv;
//      // config = cfg;
//      cgi_pipe[0] = 0;
//      cgi_pipe[1] = 0;
//  }

HttpConnection::HttpConnection(HttpConnection const &other)
    : fd(other.fd), config(other.config), serv(other.serv),
      recv_buffer(other.recv_buffer), send_buffer(other.send_buffer),
      header_str(other.header_str), body_str(other.body_str),
      last_activity(other.last_activity),
      last_cgi_activity(other.last_cgi_activity),
      content_length(other.content_length), busy(other.busy),
      is_connected(other.is_connected), is_cgi_running(other.is_cgi_running),
      cgi_response(other.cgi_response), cgi_finished(other.cgi_finished),
      cgi_result(other.cgi_result), should_die(other.should_die),
      is_response_ready(other.is_response_ready),
      is_keep_alive(other.is_keep_alive),
      header_is_parsed(other.header_is_parsed),
      body_is_read(other.body_is_read), cgi_pid(other.cgi_pid),
      is_chunked_transfer(other.is_chunked_transfer) {
  cgi_pipe[0] = other.cgi_pipe[0];
  cgi_pipe[1] = other.cgi_pipe[1];
}

HttpConnection &HttpConnection::operator=(const HttpConnection &other) {
  (void)other;
  if (this == &other) {
    return *this;
  }
  fd = other.fd;
  config = other.config;
  serv = other.serv;
  recv_buffer = other.recv_buffer;
  send_buffer = other.send_buffer;
  header_str = other.header_str;
  body_str = other.body_str;
  last_activity = other.last_activity;
  last_cgi_activity = other.last_cgi_activity;
  content_length = other.content_length;
  busy = other.busy;
  is_connected = other.is_connected;
  is_cgi_running = other.is_cgi_running;
  cgi_response = other.cgi_response;
  cgi_finished = other.cgi_finished;
  cgi_result = other.cgi_result;
  should_die = other.should_die;
  is_response_ready = other.is_response_ready;
  is_keep_alive = other.is_keep_alive;
  header_is_parsed = other.header_is_parsed;
  body_is_read = other.body_is_read;
  cgi_pid = other.cgi_pid;
  cgi_pipe[0] = other.cgi_pipe[0];
  cgi_pipe[1] = other.cgi_pipe[1];
  is_chunked_transfer = other.is_chunked_transfer;
  return *this;
}

void HttpConnection::update_last_activity() {
  last_activity = std::time(&last_activity);
}

#define DEBUG
#ifdef DEBUG
void HttpConnection::print_connection() {
  std::cout << "{";
  std::cout << fd << " ";
  // std::cout << recv_stream << " ";
  std::cout << (*config).unwrap() << " ";
  std::cout << recv_buffer << " ";
  std::cout << send_buffer << " ";
  std::cout << header_str << " ";
  std::cout << body_str << " ";
  std::cout << last_activity << " ";
  std::cout << last_cgi_activity << " ";
  std::cout << content_length << " ";
  std::cout << busy << " ";
  std::cout << is_connected << " ";
  std::cout << is_cgi_running << " ";
  std::cout << cgi_response << " ";
  std::cout << cgi_finished << " ";
  std::cout << cgi_result << " ";
  std::cout << should_die << " ";
  std::cout << is_response_ready << " ";
  std::cout << is_keep_alive << " ";
  std::cout << header_is_parsed << " ";
  std::cout << body_is_read << " ";
  std::cout << cgi_pid << " ";
  std::cout << cgi_pipe[0] << " ";
  std::cout << cgi_pipe[1] << " ";
  std::cout << is_chunked_transfer << " ";
  std::cout << recv_done << " ";
  std::cout << "}";
}
#endif
