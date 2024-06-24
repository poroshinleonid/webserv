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
  std::cout << "SERVER IS LISTENING\n";

  return 0;
}

int ServerCore::run() {
  fd_set reading_set;
  fd_set writing_set;
}