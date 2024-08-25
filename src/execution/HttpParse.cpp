#include "HttpParse.hpp"
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

HttpRequest::HttpRequest(stringstream& stream) {
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

HttpRequest::Method HttpRequest::get_method() {
    return method_;
}

string HttpRequest::get_url() {
    return url_;
}

string HttpRequest::get_body() {
    return body_;
}

string HttpRequest::get_header_at(const string& s) {
    return headers_.at(s);
}

// bad request exception //

HttpRequest::BadRequest::BadRequest(char const* const message) throw(): std::runtime_error(message) {}

char const* HttpRequest::BadRequest::what() const throw() {
    return std::runtime_error::what();
}
