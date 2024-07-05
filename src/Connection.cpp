#include "Config.hpp"
#include "Connection.hpp"

#include <iostream>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>

Connection::Connection(const std::string &ip, int port, int listen_backlog)
    : ip_str_(ip), port_(port), listen_backlog_(listen_backlog) {

}

int Connection::setup(const Config& config) {
  (void)config;
  listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd_ < 0) {
      perror("socket");
     return (-1);
  }
    server_addr_in.sin_family = AF_INET;
    server_addr_in.sin_addr.s_addr = INADDR_ANY; // FIX - add real IP that was in the config
    server_addr_in.sin_port = htons(port_);
    memset(&(server_addr_in.sin_zero), '\0', 8);

  int opt = 1;
  if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt");
    return (-1);
  }

  if (bind(listen_fd_, (struct sockaddr *)&server_addr_in, sizeof(server_addr_in)) < 0) {
      perror("bind");
      return (-1);
  }

  std::cout << "I HAVE BINDED THE SERVER\n";
  if (listen(listen_fd_, listen_backlog_) == -1)
	{
		perror("listen");
		return (-1);
	}
  FD_SET(listen_fd_, &all_fds_);
  max_fd_ = listen_fd_;
  std::cout << "SERVER IS LISTENING\n";
  return 0;
}






/* -------------------------------------------------------- */











int Connection::run(const Config& config) {
  (void)config;
  char buffer[CHUNK_SZ_FIX]; // FIX - buffer size should be dynamic depending on config or idk on something else

  int accept_fd;
  std::cout << "Starting select() loop" << std::endl;
  while (true) {
    FD_ZERO(&read_fds_);
    FD_ZERO(&write_fds_);
    FD_ZERO(&cgi_fds_);
    FD_COPY(&all_fds_, &read_fds_);
    FD_COPY(&all_fds_, &write_fds_);

    if (select(max_fd_ + 1, &read_fds_, &write_fds_, NULL, NULL) == -1) {
        perror("select"); // FIX strerror
        exit(1);
    }
    handle_fds();
  }


  return 0;
}



/* -------------------------------------------------------- */





void Connection::handle_fds() {
  for (int i = 0; i <= max_fd_; i++) {
    if (FD_ISSET(i, &read_fds_)) {
      handle_read(i);
    } else if (FD_ISSET(i, &write_fds_)&& i != listen_fd_) {
      handle_write(i);
    } else {
      handle_request_if_ready(i);
    }
  }
}

void Connection::handle_read(int fd) {
    if (fd == listen_fd_) {
      handle_accept(fd);
      return ;
    } else if (FD_ISSET(fd, &cgi_fds_)) {
      handle_cgi_output(cgi_pipes_to_sockets_[fd]);
    }
    handle_incoming_data(fd);
}

void Connection::handle_accept(int fd) {
  FD_CLR(fd, &write_fds_);
  (void)fd;
  accept_();
}

// we have some output from a cgi
// handle it (write it to a buffer)
// this function does not send anything anywhere
int Connection::handle_cgi_output(int cgi_pipe_fd) {
  FD_CLR(cgi_pipe_fd, &write_fds_);
  static char cgi_chunk[CHUNK_SZ_FIX];
  int socket_to_send = cgi_pipes_to_sockets_[cgi_pipe_fd];
  if (!(FD_ISSET(socket_to_send, &all_fds_))) {
    close(cgi_pipe_fd);
    cgi_pipes_to_sockets_.erase(cgi_pipe_fd);
    return ;
  }
  bzero(cgi_chunk, sizeof(cgi_chunk)); // FIX: needs optimization
  int read_bytes = read(cgi_pipe_fd, cgi_chunk, sizeof(cgi_chunk));
  if (read_bytes == -1) {
    if (errno == EWOULDBLOCK) {
      return 0;
    }
    return -1;
  }
  if (read_bytes < CHUNK_SZ_FIX) {
    FD_CLR(cgi_pipe_fd, &all_fds_);
    FD_CLR(cgi_pipe_fd, &cgi_fds_);
    cgi_pipes_to_sockets_.erase(cgi_pipe_fd);
  }
  write_buffers_[socket_to_send].append(cgi_chunk);
  return (read_bytes);
}

// FIX: does this function need to do anything else (???????)
void Connection::handle_incoming_data(int fd) {
  FD_CLR(fd, &write_fds_);
  //recv() CHUNK_SIZE bytes
  //append them to the read_buffer
  // ^done by recv_chunk_
  int recvd_bytes = recv_chunk_(fd);
  if (recvd_bytes == -1) {
    perror("recv");
    return ; // FIX - should return recvd bytes or something idk
  }
  return ;
}

void Connection::handle_write(int fd) {
  // if there's something in the write_buffer, send() a chunk from the buffer
    // also crop the buffer and if needed clear and do all upkeep
  // if the write_buffer is empty, call handle_request() and (possibly) fill the write_buffer
  // handle_request() should do all the upkeep (cropping read_buffer, etc) itself
}






/* -------------------------------------------------------- */



// assumes listen_fd has smth to accept and accepts it
// FIX: saves new fd everywhere, so the return value is actually useless
// saved it cuz mb well need it later
int Connection::accept_() {
  struct sockaddr_in client_addr;
  socklen_t sin_size = sizeof(client_addr);
  int new_fd = accept(listen_fd_, (struct sockaddr *)&client_addr, &sin_size);
  if (new_fd == -1) {
    if (errno == EWOULDBLOCK) {
      return 0;
    }
    perror("accept");
    return new_fd;
  }
  //update the dyn data
  FD_SET(new_fd, &all_fds_);
  if (new_fd > max_fd_) {
      max_fd_ = new_fd;
  }
  //update the dyn data structures
  read_buffers_[new_fd] = "";
  write_buffers_[new_fd] = "";
  // FIX - add log
  std::cout << "New connection from " << inet_ntoa(client_addr.sin_addr) << " on socket " << new_fd << std::endl;
  return new_fd;
}


// like ::recv() but only a chunk and save it to a buffer
// you only give it an fd, it already knows what to send
// buffers should be filled by other functions
int Connection::recv_chunk_(int fd) {
  static char recv_chunk[CHUNK_SZ_FIX];
  bzero((void *)recv_chunk, sizeof(recv_chunk)); // FIX - just add \0 after reading to save time!
  int recvd_bytes = recv(fd, recv_chunk, sizeof(recv_chunk - 1), 0);
  if (recvd_bytes < 0) {
    return recvd_bytes;
  }
  if (recvd_bytes == 0) {
    std::cout << "socket " << fd << " hung up" << std::endl;
    //NB: erasing socket from map and deletinf from fd_sets should be done by a caller!!!
    return 0;
  }
  read_buffers_[fd].append(recv_chunk);
  return recvd_bytes;
}

// like ::send() but only a chunk and save it to a buffer
// you only give it an fd, it already knows what to send
// buffers should be read and processed by other functions
int Connection::send_chunk_(int fd) {
  static char send_chunk[CHUNK_SZ_FIX];
  bzero((void *)send_chunk, sizeof(send_chunk));
  std::string &s = write_buffers_[fd]; // sugar
  if (s.empty()) {
    return 0;
  }
  const char *tmp_str = s.c_str();
  int cur_chunk_sz =  ((s.length() > CHUNK_SZ_FIX)?(CHUNK_SZ_FIX):(s.length()));
  int sent_bytes = send(fd, tmp_str, cur_chunk_sz, 0);
  if (sent_bytes != -1) {
    s.erase(0, sent_bytes);
  }
  return sent_bytes;
}




/*
since there's no trivial way to tell if I recieved all of the http request,
i should just send the read_buffer to httprequesthandler every time i have nothing else to do
and it will extract the necessary data from it and crop/slice the read_buffer if needed
it will also fill write_buffer if it can
so I can just call handle_request_if_ready(fd) and assume everything is good (unless it returns a bad return code)
*/

//this function can be called after we 
// 1. AND can send() to a socket AND
// 2. AND found a complete http request (rnrn) in its buffer
// This function assumes that select() said you can't  read from this fd
// so we check if there's a fully recieved request
// and evaluate the request synchronously by other modules
// and then save the result into a buffer so it can be sent later
// handle_request() should do all the upkeep (cropping read_buffer, etc) itself
int Connection::handle_request_if_ready(int fd) {
    //if rnrn found, crop it from the buffer and send synchronously to requesthandler
    //recieve a string from requesthandler and write it into the write_buffer
    //somehow we need to track if our response was sent to CGI, in which case save FD to 
      //to all maps and continue.
  //if not found, continue;
}