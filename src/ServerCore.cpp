#include "ServerCore.hpp"

ServerCore::ServerCore() {}

ServerCore::ServerCore(ServerCore const &other) {(void)other;};

ServerCore::~ServerCore() {}

ServerCore	&ServerCore::operator=(const ServerCore &other) {
  if (this != &other) {
  }
  return (*this);
}

ServerCore::ServerCore(const std::string &ip, int port, int listen_backlog)
    : ip_string_(ip), port_(port), listen_backlog_(listen_backlog) {

}

int ServerCore::setup() {
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
      perror("socket");
     return (-1);
  }
    server_addr_in.sin_family = AF_INET;
    server_addr_in.sin_addr.s_addr = INADDR_ANY; // FIX - add real IP that was in the config
    server_addr_in.sin_port = htons(port_);
    memset(&(server_addr_in.sin_zero), '\0', 8);

  if (bind(listen_fd, (struct sockaddr *)&server_addr_in, sizeof(server_addr_in)) < 0) {
      perror("bind");
      return (-1);
  }
  std::cout << "I HAVE BINDED THE SERVER\n";
  if (listen(listen_fd, listen_backlog_) == -1)
	{
		perror("listen");
		return (-1);
	}
  FD_SET(listen_fd, &reading_set_);
  FD_SET(listen_fd, &writing_set_);
  FD_SET(listen_fd, &all_fds_set_);
  std::cout << "SERVER IS LISTENING\n";

  return 0;
}

int ServerCore::accept_(int new_fd) {
  (void) new_fd;
  return (0);
}
int ServerCore::recv_buffer_(int new_fd) {
  (void) new_fd;
  return (0);

}
int ServerCore::send_buffer_(int new_fd) {
  (void) new_fd;
  return (0);

}

//ACHTuNG
//I NEED TO ReAD IT IN  CHUNKS
// ALGO
//    1. Read chunk by chunk
//    2. When the whole request is read, start process()
//    3. process() will form the whole response 
//              (program will be blocked while it does it, except for CGI)
//    4. process() will save response to write_buffer
//    5. continue, but not just read chunks: also write chunks if needed



// send_all_buffers(fd_set write_ready_fds)
// Takes an fd_set of buffers that are ready to be written to
// For each buffer in this set if there's any data that needs to be sent
// then send a chunk (buff_size) and continue;



// I need an fd_set that will store all of the fds that have a response for them
// What info do I even give to the response generator?
// let's say it is an FD number
// so response generator will give me two items:
// 1. struct response
// 2. fd number
// so I will store both fd_set (for macros), and a map in a  {fd:s_response} format


int ServerCore::run() {

  struct sockaddr_in client_addr;
  socklen_t sin_size;
  char buffer[BUFF_SIZE_SERV];

  fd_set cur_read_fds, cur_write_fds;
  int maxfd = listen_fd;
  int new_fd;
  std::cout << "RUNNING\n";
  while (true) {
    FD_ZERO(&cur_read_fds);
    FD_ZERO(&cur_write_fds);
    FD_COPY(&all_fds_set_, &cur_read_fds);
    FD_COPY(&all_fds_set_, &cur_write_fds);

    if (select(maxfd + 1, &cur_read_fds, &cur_write_fds, NULL, NULL) == -1) {
        perror("select");
        std::cout << "SELECTION ERROR LMAO";
        exit(1);
    }


    // std::cout << "Something was selected!" << std::endl;
    // fd starts with 3 because 0 = stdin, 1 =stdout, 2 = stderr
    for (int i = 3; i <= maxfd; i++) {

      if (FD_ISSET(i, &cur_read_fds)) {
        std::cout << "Something was selected to READ!" << std::endl;
        if (i == listen_fd) {
          //need to accept new connection!
          sin_size = sizeof(client_addr);
          new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &sin_size);
          if (new_fd == -1) {
              if (errno != EWOULDBLOCK) {
                  perror("accept");
              }
          } else {
              FD_SET(new_fd, &all_fds_set_); // addd the new socket to the master set
              if (new_fd > maxfd) {
                  maxfd = new_fd;
              }
              printf("New connection from %s on socket %d\n",
                      inet_ntoa(client_addr.sin_addr), new_fd);
          }
        } else {
              // need to work on input from the client
              std::cout << "input reading\n";

            int nbytes = recv(i, buffer, sizeof(buffer) - 1, 0);
            if (nbytes <= 0) {
                              if (nbytes == 0) {
                                  printf("Socket %d hung up\n", i);
                              } else {
                                  perror("recv");
                              }
                              close(i);           
                              FD_CLR(i, &all_fds_set_); 
            } else {
              // printf data that was recieved from the slient
              buffer[nbytes] = '\0';
              printf("Received: %s", buffer);
            }
          continue;
        }
      //  } else if (FD_ISSET(i, &cur_read_fds) && FD_ISSET(i, &cur_write_fds)) { // elif
       } else if (FD_ISSET(i, &cur_write_fds)) { // elif
        std::cout << "Something was selected to WRITE!" << std::endl;
        // There's no input from the client, we can send some data
        // I think if we still have any data to read, we cannot write
        // if we can, just replace elif with just if
        // Send a simple HTTP response
        if (i == listen_fd) {
          continue;
        }
        std::cout << "Trying to write onto fd " << i << std::endl;
        const char *response = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: 12\r\n"
                              "\r\n"
                              "Hello world!";
        if (send(i, response, strlen(response), 0) == -1) {
            perror("send");
        }
      }


    }
  }
  return 0;
}