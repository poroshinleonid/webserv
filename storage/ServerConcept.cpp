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