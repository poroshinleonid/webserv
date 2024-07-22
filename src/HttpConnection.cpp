#include "HttpConnection.hpp"
#include "Server.hpp"

HttpConnection::~HttpConnection() {
  if (cgi_pipe[0] != 0) {
    close(cgi_pipe[0])
  }
  if (cgi_pipe[1] != 1) {
    close(cgi_pipe[1])
  }
}


HttpConnection::HttpConnection(Config &cfg, Server &srv)
  : config(cfg), serv(srv) {
  (void)cfg;
  (void)srv;
  content-length = -1;
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

//FIX remove?
HttpConnection::HttpConnection(int fd, Server &scfg, Config &cfg)  :
    fd(fd), port(scfg.port), host(scfg.host), config(cfg),
    recv_buffer(""), send_buffer(""),
    header_str(""), body_str(""), content_length(0),
    busy(false), is_connected(true), 
    is_cgi_running(false), 
    cgi_response(""),
    cgi_finished(false), should_die(false),
    is_response_ready(false),
    is_keep_alive(false), header_is_parsed(false), 
    body_is_read(false), cgi_pid(0) {
    last_activity = std::time(&last_activity);
    (void)fd;
    (void)scfg;
    // config = cfg;
    cgi_pipe[0] = 0;
    cgi_pipe[1] = 0;
}

HttpConnection::HttpConnection(HttpConnection const &other) :config(other.config) {
  (void)other;
};

HttpConnection::~HttpConnection() {}

HttpConnection	&HttpConnection::operator=(const HttpConnection &other) {
  (void)other;
  if (this != &other) {
  }
  return (*this);
}


void HttpConnection::update_last_activity() {
  last_activity = std::time(&last_activity);
}
