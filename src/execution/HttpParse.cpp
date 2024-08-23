#include "HttpParse.hpp"
#include "Base.hpp"

#define CRLF "\r\n"

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
    if (line.find('/') == string::npos) {
        throw BadRequest("Bad request header");
    }
    method_ = ::get_method(trim(line.substr(0, line.find('/'))));
    check_http_version(trim(line.substr(line.find('/') + 1)));
    for (; stream >> line; line != CRLF) {

    }
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



// bad request exception //

HttpRequest::BadRequest::BadRequest(char const* const message) throw(): std::runtime_error(message) {}

char const* HttpRequest::BadRequest::what() const throw() {
    return std::runtime_error::what();
}
