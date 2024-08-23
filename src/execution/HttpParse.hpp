#pragma once

#include <string>
#include <unordered_map>
#include <sstream>
#include <array>

class HttpRequest {
  using string = std::string;
  using stringstream = std::stringstream;
  public:
    static const std::array<string, 2> allowedHttpVersions;
    enum class Method {GET, POST, DELETE};
    HttpRequest(stringstream& stream);
    HttpRequest(const HttpRequest& other) = default;
    HttpRequest& operator=(const HttpRequest& other) = default;
    ~HttpRequest() = default;

    Method get_method();
    string operator[](const string& s);
    string get_body();
  private:
    Method method_ = Method::GET;
    std::unordered_map<string, string> headers_;
    string body_;
  public:
    class BadRequest: std::runtime_error {
        public:
            BadRequest(char const* const message) throw();
            virtual char const* what() const throw();
    };
};
