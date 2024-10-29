#pragma once

#include <array>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <climits>

#define CRLF "\r\n"
#define CRLFCRLF "\r\n\r\n"
#define CHUNKTERM "0\r\n\r\n"
#define SP ' '

class HttpRequest {
  using string = std::string;
  using stringstream = std::stringstream;

public:
  static const int MAX_BODY_SIZE = 10'000;
  static const std::array<string, 2> allowedHttpVersions;
  static std::vector<std::string> parse_url(const std::string &url);
  static std::string join_url(const std::vector<std::string> &parsed_url);
  enum class Method { GET, POST, DELETE, HEAD };
  HttpRequest();
  HttpRequest(const string &s);
  HttpRequest(const HttpRequest &other) = default;
  HttpRequest &operator=(const HttpRequest &other) = default;
  ~HttpRequest() = default;

  Method get_method() const;
  static std::string method_to_str(const Method &method);
  string get_url() const;
  string get_header_at(const string &s) const;
  string get_header() const;
  const std::unordered_map<string, string> &get_header_map() const;
  string get_body() const;
  string get_response_str() const;
  std::string get_host() const; // throws if none
  int get_port() const;         // 80 if none, throws if not int / negative
private:
  Method method_ = Method::GET;
  string url_;
  std::unordered_map<string, string> headers_;
  string body_;
  string header_;
  string response_str_;

public:
  class BadRequest : std::runtime_error {
  public:
    BadRequest(char const *const message) throw();
    virtual char const *what() const throw();
  };
  class RequestNotFinished : std::runtime_error {
  public:
    RequestNotFinished(char const *const message) throw();
    virtual char const *what() const throw();
  };
};
