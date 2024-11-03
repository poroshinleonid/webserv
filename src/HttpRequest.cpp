#include "HttpRequest.hpp"
#include "Base.hpp"
#include "Libft.hpp"
#include <algorithm>

using std::string, std::stringstream, std::unordered_map;

#define MAX_URL_LEN 2000

const std::array<std::string, 2> HttpRequest::allowedHttpVersions = {
    "HTTP/1.1", "HTTP/1.0"};

namespace {
HttpRequest::Method get_method(const std::string &method) {
  if (method == "GET") {
    return HttpRequest::Method::GET;
  } else if (method == "POST") {
    return HttpRequest::Method::POST;
  } else if (method == "DELETE") {
    return HttpRequest::Method::DELETE;
  } else if (method == "HEAD") {
    return HttpRequest::Method::HEAD;
  } else {
    throw HttpRequest::BadRequest("bad http method");
  }
}

void check_http_version(const std::string &version) {
  for (const string &s : HttpRequest::allowedHttpVersions) {
    if (version == s) {
      return;
    }
  }
  throw HttpRequest::BadRequest("unsupported or invalid http version");
}
} // namespace

HttpRequest::HttpRequest() : HttpRequest("GET / HTTP/1.1\r\n\r\n") {}

HttpRequest::HttpRequest(const string &s) {
  response_str_ = s;
  std::stringstream stream(s);
  std::string line;
  if (!getline_str(stream, line, CRLF)) {
    if (line.size() >= MAX_URL_LEN) {
      throw UriTooLong("Uri in the header is too long");
    }
    throw RequestNotFinished("Not finished chunked request");
  }
  // getline_str(stream, line, CRLF);
  size_t method_end = line.find(SP);
  size_t url_start = method_end;
  for (; url_start < line.size() && line[url_start] == ' '; url_start++) {
  }
  size_t url_end = line.find(SP, url_start);
  if (method_end == string::npos || url_end == string::npos) {
    throw BadRequest("Bad request header");
  }
  method_ = ::get_method(line.substr(0, method_end));
  url_ = trim(line.substr(url_start, url_end - url_start));
  check_http_version(trim(line.substr(url_end + 1)));
  if (s.find(CRLFCRLF) == std::string::npos && s.find("Transfer-Ecnoding") == std::string::npos) {
    throw RequestNotFinished("not chunked");
  }
  while (true) {
    getline_str(stream, line, CRLF);
    if (line == "") {
      break;
    }
    string raw_key = trim(line.substr(0, line.find(":")));
    string key = "";
    std::transform(raw_key.begin(), raw_key.end(), std::back_inserter(key), Libft::tolower);
    string value = "";
    string raw_value = "";
    if (line.find(":") != std::string::npos) {
      raw_value = trim(line.substr(line.find(":") + 1));
      std::transform(raw_value.begin(), raw_value.end(), std::back_inserter(value), Libft::tolower);
    }
    headers_[key] = value;
  }
  if (method_ == Method::GET || method_ == Method::DELETE || method_ == Method::HEAD) {
    return;
  }
  header_ = s.substr(0, s.find(CRLFCRLF));
  std::string body;
  if (headers_.find("transfer-encoding") != headers_.end() &&
      headers_.at("transfer-encoding") == "chunked") {
    while (true) {
      std::string chunk_size_line;
      if (!getline_str(stream, chunk_size_line, CRLF)) {
        throw RequestNotFinished("Not finished chunked request");
      }
      int chunk_size;
      try {
        size_t pos;
        chunk_size = std::stoi(chunk_size_line, &pos);
        if (pos != chunk_size_line.size()) {
          std::cout << "chunk_line_size = " << chunk_size_line;
          std::cout << "; chunk_size = " << chunk_size;
          std::cout << "; pos = " << pos << std::endl;
          throw std::invalid_argument("The whole string should be a number");
        }
      } catch (std::invalid_argument &) {
        throw BadRequest("Expected an integer as a chunk size");
      }
      if (chunk_size == 0) {
        body_ = body;
        return;
      }
      for (int i = 0; i < chunk_size; i++) {
        char c;
        if (!stream.get(c)) {
          throw BadRequest("Chunk size was too short");
        }
        body += c;
      }
    }
  }
  body_ = s.substr(s.find(CRLFCRLF), s.find(CRLFCRLF, s.find(CRLFCRLF)));
}

HttpRequest::Method HttpRequest::get_method() const { return method_; }

std::string HttpRequest::method_to_str(const Method &method) {
  switch (method) {
  case Method::GET:
    return "GET";
  case Method::POST:
    return "POST";
  case Method::DELETE:
    return "DELETE";
  case Method::HEAD:
    return "HEAD";
  default:
    return "";
  }
}

string HttpRequest::get_url() const { return url_; }

string HttpRequest::get_body() const { return body_; }
string HttpRequest::get_response_str() const { return response_str_; }

string HttpRequest::get_host() const {
  string host_full = get_header_at("host");
  std::vector<string> host_split = split_one(host_full, ':');
  return host_split.at(0);
}

int HttpRequest::get_port() const {
  string host_full = get_header_at("host");
  std::vector<string> host_split = split_one(host_full, ':');
  if (host_split.size() < 2) {
    return 80;
  }
  string host_str = host_split[1];
  int port = std::stoi(host_str);
  if (port < 0) {
    throw std::invalid_argument("port can't be negative");
  }
  return port;
}

string HttpRequest::get_header_at(const string &s) const {
  return headers_.at(s);
}

string HttpRequest::get_header() const {
  return header_;
}

const std::unordered_map<string, string> &HttpRequest::get_header_map() const {
  return headers_;
}


HttpRequest::BadRequest::BadRequest(char const *const message) throw()
    : std::runtime_error(message) {}

char const *HttpRequest::BadRequest::what() const throw() {
  return std::runtime_error::what();
}

HttpRequest::RequestNotFinished::RequestNotFinished(
    char const *const message) throw()
    : std::runtime_error(message) {}

char const *HttpRequest::RequestNotFinished::what() const throw() {
  return std::runtime_error::what();
}

HttpRequest::UriTooLong::UriTooLong(
    char const *const message) throw()
    : std::runtime_error(message) {}

char const *HttpRequest::UriTooLong::what() const throw() {
  return std::runtime_error::what();
}

std::vector<std::string> HttpRequest::parse_url(const std::string &url) {
  std::vector<std::string> parsed_url = split(url, '/');
  if (url.size() != 0 && url[0] == '/') {
    parsed_url.erase(parsed_url.begin());
  }
  return parsed_url;
  // TODO: port (or not)
}

std::string HttpRequest::join_url(const std::vector<std::string> &parsed_url) {
  std::string result;
  for (auto it = parsed_url.begin(); it < parsed_url.end(); it++) {
    result += *it;
    if (it + 1 != parsed_url.end()) {
      result += "/";
    }
  }
  return result;
}
