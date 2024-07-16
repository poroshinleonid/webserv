#include "HttpConnection.hpp"
#include "ServerConfig.hpp"

HttpConnection::HttpConnection(Config &cfg) :config(cfg) {
  (void)cfg;
}

HttpConnection::HttpConnection(int fd, ServerConfig &scfg, Config &cfg)  :
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
