#include "Server.hpp"

Server::Server() {}

Server::~Server() {}

Server::Server(Server const &other) 
:port(other.port), host(other.host), srv_sockaddr(other.srv_sockaddr),
timeout(other.timeout), listen_fd(other.listen_fd), is_default(other.is_default),
server_name(other.server_name), default_error_pages(other.default_error_pages),
client_body_size(other.client_body_size), routes(other.routes) {

}

Server &Server::operator=(const Server &other) {
	if (this == &other) {
		return *this;
	}
	port = other.port;
	host = other.host;
	srv_sockaddr = other.srv_sockaddr;
	timeout = other.timeout;
	listen_fd = other.listen_fd;
	is_default = other.is_default;
	server_name = other.server_name;
	default_error_pages = other.default_error_pages;
	client_body_size = other.client_body_size;
	routes = other.routes;
	return *this;
}
