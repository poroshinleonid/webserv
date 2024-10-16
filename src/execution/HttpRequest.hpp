#pragma once

#include <string>
#include <unordered_map>
#include <sstream>
#include <array>

class HttpRequest {
  using string = std::string;
  using stringstream = std::stringstream;
  public:
    static const int MAX_BODY_SIZE = 1 * 1000 * 1000;
    static const std::array<string, 2> allowedHttpVersions;
    static std::vector<std::string> parse_url(const std::string& url);
    enum class Method {GET, POST, DELETE};
    HttpRequest();
    HttpRequest(const string& s);
    HttpRequest(const HttpRequest& other) = default;
    HttpRequest& operator=(const HttpRequest& other) = default;
    ~HttpRequest() = default;

    Method get_method() const;
    static std::string method_to_str(const Method& method);
    string get_url() const;
    string get_header_at(const string& s) const;
    string get_body() const;
    std::string get_host() const; // throws if none
    int get_port() const; // 80 if none, throws if not int / negative
  private:
    Method method_ = Method::GET;
    string url_;
    std::unordered_map<string, string> headers_;
    string body_;
  public:
    class BadRequest: std::runtime_error {
        public:
            BadRequest(char const* const message) throw();
            virtual char const* what() const throw();
    };
};

