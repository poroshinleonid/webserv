fd listen_fd
fd_set all_fds // also contains listen_fd
map <fd, struct_connection>

// add_fd() and remove_fd() should add/remove BOTH from add_fds set AND from map.

struct connection {
  int sock_fd;
  int cgi_fd;
  int fd_type // enum PIPE, SOCKET, LISTEN
  std::string request_string; // will be filled chunk by chunk
  std::string response_string;  // will be filled either at once by webserv module
                                // or chunk by chunk from a pipe to CGI
   
}

int listen_fd;
df_set socket_fds; (contains listen_fd);
map<int, int> - associate a pipe with socket_fd;


we have fd_set that contains:
  1. listen_fd
  2. connection fds, some ready to read, some ready to write to
  3. pipe fds to read a response from the CGI


1. select
2.1 if new connection:
  1. create a socket for it
  2. add new socket to the list
  3. update max_socket
  4. continue
2.2 if new data to read from a connection:
  1. read the chunk to the buffer
  2. if the request is read in its entirety, form a response.
    1. if CGI no needed, call the function and save the response
    2. if CGI needed, call CGI and add a pipe fd, and associate it with the socket fd
    3. if request says "we're done", close the socket and clean up everything
2.3 if new data to read from a pipe
  1. read a chunk from this pipe into associated socket's send_buffer'
  2. If pipe is empty and done, close the pipe
2.4 if socket is ready to write to
  1. if there's data ready, send_buff() a chunk of this data