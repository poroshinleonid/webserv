#ifndef HttpRequest_HPP
#define HttpRequest_HPP

#include <string>
#include <map>

enum http_method {
  HTTP_METH_GET,
  HTTP_METH_HEAD,
  HTTP_METH_POST,
  HTTP_METH_PUT,
  HTTP_METH_DELETE,
  HTTP_METH_CONNECT,
  HTTP_METH_OPTIONS,
  HTTP_METH_TRACE,
  HTTP_METH_PATCH,
};

class	HttpRequest {
public:
  HttpRequest();
  HttpRequest(HttpRequest const &other);
  ~HttpRequest();
  HttpRequest &operator=(const HttpRequest &obj);

public:
  HttpRequest(const std::string &request_string);


public:
  /**
   * @brief convert the whole request to a string
   *
   * @return std::string 
   */
  std::string to_string() const;


// internal workings data
public:
  bool is_valid;
  bool is_for_cgi;
  std::string client_hostname;
  std::string client_port;

// Actual request fields
public:
  std::string header;
  std::string body;
  http_method method;
  std::string uri;
  std::string version;
  std::map<std::string, std::string> header_params;
  std::map<std::string, std::string> query_params;



};

#endif
