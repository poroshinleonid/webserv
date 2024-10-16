#include "HttpRequest.hpp"
#include "Base.hpp"

#define CRLF "\r\n"
#define SP ' '

using std::string, std::stringstream, std::unordered_map;

const std::array<std::string, 2> HttpRequest::allowedHttpVersions = {"HTTP/1.1", "HTTP/1.0"};

namespace {
    HttpRequest::Method get_method(const std::string& method) {
        if (method == "GET") {
            return HttpRequest::Method::GET;
        } else if (method == "POST") {
            return HttpRequest::Method::POST;
        } else if (method == "DELETE") {
            return HttpRequest::Method::DELETE;
        } else {
            throw HttpRequest::BadRequest("bad http method");
        }
    }

    void check_http_version(const std::string& version) {
        for (const string& s : HttpRequest::allowedHttpVersions) {
            if (version == s) {
                return;
            }
        }
        throw HttpRequest::BadRequest("unsupported or invalid http version");
    }
}

HttpRequest::HttpRequest(): HttpRequest("GET / HTTP/1.1\n") {}

HttpRequest::HttpRequest(const string& s) {
    std::stringstream stream(s);
    string line;
    getline_str(stream, line, CRLF);
    size_t method_end = line.find(SP);
    size_t url_start = method_end;
    for (; url_start < line.size() && line[url_start] == ' '; url_start++) {}
    size_t url_end = line.find(SP, url_start);
    if (method_end == string::npos || url_end == string::npos) {
        throw BadRequest("Bad request header");
    }
    method_ = ::get_method(line.substr(0, method_end));
    url_ = trim(line.substr(url_start, url_end - url_start));
    check_http_version(trim(line.substr(url_end + 1)));
    while (true) {
        getline_str(stream, line, CRLF);
        if (line == "") {
            break;
        }
        string key = trim(line.substr(0, line.find(":")));
        string value = "";
        if (line.find(":") != std::string::npos) {
            value = trim(line.substr(line.find(":") + 1));
        }
        headers_[key] = value;
    }
    std::string body;
    while (getline_str(stream, line, CRLF)) {
        if (line == "") {
            break;
        }
        body += line;
    }
    body_ = body;
}

HttpRequest::Method HttpRequest::get_method() const {
    return method_;
}

std::string HttpRequest::method_to_str(const Method& method) {
    switch (method) {
        case Method::GET:
            return "GET";
        case Method::POST:
            return "POST";
        case Method::DELETE:
            return "DELETE";
    }
}

string HttpRequest::get_url() const {
    return url_;
}

string HttpRequest::get_body() const {
    return body_;
}

string HttpRequest::get_host() const {
    string host_full = get_header_at("Host");
    std::vector<string> host_split = split_one(host_full, ':');
    return host_split.at(0);
}

int HttpRequest::get_port() const {
    string host_full = get_header_at("Host");
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

string HttpRequest::get_header_at(const string& s) const {
    return headers_.at(s);
}

HttpRequest::BadRequest::BadRequest(char const* const message) throw(): std::runtime_error(message) {}

char const* HttpRequest::BadRequest::what() const throw() {
    return std::runtime_error::what();
}

std::vector<std::string> HttpRequest::parse_url(const std::string& url) {
    std::vector<std::string> parsed_url = split(url, '/');
    if (url.size() != 0 && url[0] == '/') {
        parsed_url.erase(parsed_url.begin());
    }
    return parsed_url;
    // TODO: port (or not)
}

std::string HttpRequest::join_url(const std::vector<std::string>& parsed_url) {
    std::string result;
    for (const std::string& s : parsed_url) {
        result += s;
        result += "/";
    }
    return result;
}
