#ifndef Server_HPP
#define Server_HPP

#include <string>
#include <map>
#include <vector>

struct CgiInfo {
  std::string path;
  std::string extension;
};

struct Route {
  std::string route_string;
  std::vector<std::string> methods; // FIX - add enum
  bool is_redirected;
  std::string redirection;
  std::string root_directory; 
  bool directory_listing_enabled;
  std::string default_file_for_dir_request_path;
  std::string cgi_info;
  //each route should work with post and get methods
  // Make the route able to accept uploaded files and configure where they should be saved.
  //The previous rule's explanation in the subj KIND OF implies we use CGI for that but idk
};

class	Server {
public:
  Server();
  Server(Server const &other);
  ~Server();
  Server &operator=(const Server &obj);

public:
  int port;
  std::string host;
  int listen_fd;
  bool is_default;
  std::string server_name;
  std::map<int, std::string> default_error_pages;
  size_t client_body_size;
  std::vector<Route> routes;

};

#endif
