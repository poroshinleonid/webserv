#include "HttpConnection.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "HttpRequest.hpp"

#include <ctime>
#include <unistd.h>

HttpConnection::HttpConnection()
    : fd(0), config(NULL), logger(NULL), serv(NULL), last_activity(0),
      last_cgi_activity(0), content_length(-1), is_connected(false),
      is_cgi_running(false), cgi_finished(false), cgi_result(false),
      socket_closed(false), is_response_ready(false), is_keep_alive(false),
      close_after_send(false), reading_garbage_chunks(false), cgi_pid(0),
      is_chunked_transfer(false), recv_done(false) {
  cgi_pipe[0] = 0;
  cgi_pipe[1] = 0;
}

HttpConnection::~HttpConnection() {
  // if (cgi_pipe[0] != 0){
  //   close(cgi_pipe[0]);
  // }
  // if (cgi_pipe[1] != 1){
  //   close(cgi_pipe[1]);
  // }
}

HttpConnection::HttpConnection(Config *cfg, Logger *log, Server *srv)
    : fd(0), config(cfg), logger(log), serv(srv), last_activity(0),
      last_cgi_activity(0), content_length(-1), is_connected(false),
      is_cgi_running(false), cgi_finished(false), cgi_result(false),
      socket_closed(false), is_response_ready(false), is_keep_alive(false),
      close_after_send(false), reading_garbage_chunks(false), cgi_pid(0),
      is_chunked_transfer(false), recv_done(false) {
  (void)cfg;
  (void)srv;
  cgi_pipe[0] = 0;
  cgi_pipe[1] = 0;
}

HttpConnection::HttpConnection(HttpConnection const &other)
    : fd(other.fd), config(other.config), logger(other.logger),
      serv(other.serv), recv_buffer(other.recv_buffer),
      send_buffer(other.send_buffer), chunked_chunk(other.chunked_chunk),
      next_requests_str(other.next_requests_str), last_activity(other.last_activity),
      last_cgi_activity(other.last_cgi_activity),
      content_length(other.content_length), is_connected(other.is_connected),
      is_cgi_running(other.is_cgi_running), cgi_response(other.cgi_response),
      cgi_finished(other.cgi_finished), cgi_result(other.cgi_result),
      socket_closed(other.socket_closed),
      is_response_ready(other.is_response_ready),
      is_keep_alive(other.is_keep_alive),
      close_after_send(other.close_after_send),
      reading_garbage_chunks(other.reading_garbage_chunks), cgi_pid(other.cgi_pid),
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
  logger = other.logger;
  serv = other.serv;
  recv_buffer = other.recv_buffer;
  send_buffer = other.send_buffer;
  chunked_chunk = other.chunked_chunk;
  next_requests_str = other.next_requests_str;
  last_activity = other.last_activity;
  last_cgi_activity = other.last_cgi_activity;
  content_length = other.content_length;
  is_connected = other.is_connected;
  is_cgi_running = other.is_cgi_running;
  cgi_response = other.cgi_response;
  cgi_finished = other.cgi_finished;
  cgi_result = other.cgi_result;
  socket_closed = other.socket_closed;
  is_response_ready = other.is_response_ready;
  is_keep_alive = other.is_keep_alive;
  close_after_send = other.close_after_send;
  reading_garbage_chunks = other.reading_garbage_chunks;
  cgi_pid = other.cgi_pid;
  cgi_pipe[0] = other.cgi_pipe[0];
  cgi_pipe[1] = other.cgi_pipe[1];
  is_chunked_transfer = other.is_chunked_transfer;
  return *this;
}

void HttpConnection::update_last_activity() {
  last_activity = std::time(&last_activity);
}

void HttpConnection::update_last_cgi_activity() {
  last_cgi_activity = std::time(&last_cgi_activity);
}

HttpChunk HttpConnection::parse_http_chunk() {
  std::string &data = recv_chunk;
  HttpChunk chunk;
  if (data.find(CHUNKTERM) == 0) {
    std::cout << "CHUNKTERM AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" << std::endl;
    data.erase(0, 5);
    next_requests_str = data;
    data.clear();
    chunk.state = HttpChunkState::END_CHUNK;
    chunk.content = "\r\n\r\n";
    // chunk.content = "\r\n\r\n";
    return chunk;
  }
  do {
    size_t hex_end_pos = data.find(CRLFCRLF);
    if (hex_end_pos == std::string::npos) {
      break;
    }
    std::string chunk_sz_hex = data.substr(0, hex_end_pos);
    size_t chunk_size;
    try {
      std::istringstream(chunk_sz_hex) >> std::hex >> chunk_size;
    } catch (...) {
      break;
    }
    if (data.size() < hex_end_pos + 2 + chunk_size) {
      break;
    }
    data.erase(0, hex_end_pos + 2);
    if (data.find(CRLFCRLF) != chunk_size) {
      break;
    }
    chunk.content = data.substr(0, chunk_size);
    data.erase(0, chunk_size + 2);
    data.clear();
    chunk.state = HttpChunkState::DATA_CHUNK;
    return chunk;
  } while (false);
  chunk.state = HttpChunkState::NOT_A_CHUNK;
  return chunk;
}


#define DEBUG
#ifdef DEBUG
void HttpConnection::print_connection() {
  std::cout << "{";
  std::cout << fd << " ";
  std::cout << (*config).unwrap() << " ";
  std::cout << recv_buffer << " ";
  std::cout << send_buffer << " ";
  std::cout << next_requests_str << " ";
  std::cout << last_activity << " ";
  std::cout << last_cgi_activity << " ";
  std::cout << content_length << " ";
  std::cout << is_connected << " ";
  std::cout << is_cgi_running << " ";
  std::cout << cgi_response << " ";
  std::cout << cgi_finished << " ";
  std::cout << cgi_result << " ";
  std::cout << socket_closed << " ";
  std::cout << is_response_ready << " ";
  std::cout << is_keep_alive << " ";
  std::cout << close_after_send << " ";
  std::cout << reading_garbage_chunks << " ";
  std::cout << cgi_pid << " ";
  std::cout << cgi_pipe[0] << " ";
  std::cout << cgi_pipe[1] << " ";
  std::cout << is_chunked_transfer << " ";
  std::cout << recv_done << " ";
  std::cout << "}";
}
#endif
