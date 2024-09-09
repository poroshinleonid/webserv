#ifndef Server_HPP
#define Server_HPP

#include <map>
#include <netinet/in.h>
#include <string>
#include <vector>

struct CgiInfo {
  std::string path;
  std::string extension;
};

struct ErrorPage {
  int page_code;
  std::string html_file;
  std::string location;
  bool internal;
};

// each route should work with post and get methods
//  Make the route able to accept uploaded files and configure where they should
//  be saved.
// The previous rule's explanation in the subj KIND OF implies we use CGI for
// that but idk
struct Route {
  std::string location; // the route string // default: REQUIRED
  std::vector<std::string>
      methods;             // accessible methods for this route // default: GET
  std::string redirection; // empty if no redirection // default: none
  // std::string modifier; // dummy for now // default: none
  std::string root_dir;   // a directory or a file from where the file should be
                          // searched (for example, if url /kapouet is rooted to
                          // /tmp/www, url /kapouet/pouic/toto/pouet is
                          // /tmp/www/pouic/toto/pouet). // default: server root
  std::string index;      // default index file // default: index.html
  std::string error_page; // default error page file // default: error.html??
  // std::vector <std::string> try_files; // dummy for now - try to server these
  // files in order. // default: {}
  bool dir_listing; // enable public directory listing, false by default /
                    // /default: false
  std::string default_file_for_dir_request_path; // what to show when directory
                                                 // is accessed default: server
                                                 // no_listing error page
  std::string cgi_extension; // extension of the requested file to call CGI //
                             // default: none
  std::string upload_files_route; // where to upload (i.e. POST????) files //
                                  // default: none
};

class Server {
public:
  Server(Server const &other);
  Server &operator=(const Server &obj);

public:
  Server();
  ~Server();

public: // read from config
  struct sockaddr_in srv_sockaddr;
  std::vector<std::string> server_names; // - split by " "
  std::vector<ErrorPage> error_pages;
  int client_max_body_size;
  std::vector<Route> routes;

public: // assign later
  unsigned int port;
  std::string host;
  double timeout;
  double cgi_timeout;
  int listen_fd;
  bool is_default;
  std::string server_name;
  std::map<int, std::string> default_error_pages;
  size_t client_body_size;

#define DEBUG
#ifdef DEBUG
public:
  void print_server();
#endif
};

#endif
