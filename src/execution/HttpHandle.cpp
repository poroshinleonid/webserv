#include "HttpHandle.hpp"
#include "HttpRequest.hpp"
#include "Base.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

std::string HttpHandle::compose_response(const std::string& request_str, Config& config) {
    HttpRequest request;

    try {
        request = HttpRequest(request_str);
    } catch (HttpRequest::BadRequest) {
        return status_code_to_response(400, config /*dummy*/);
    }

    if (request.get_body().size() > HttpRequest::MAX_BODY_SIZE) {
        return status_code_to_response(413, config /*dummy*/);
    }

    try {
        request.get_host();
        request.get_port();
    } catch (std::exception) {
        return status_code_to_response(400, config /*dummy*/);
    }

    Config server_config;
    try {
        server_config = select_server_config(request, config);
    } catch (std::exception) {
        return status_code_to_response(404, config /*dummy*/);
    }

    std::string url = request.get_url();
    Config url_config;
    try {
        url_config = select_url_config(url, server_config);        
    } catch (std::exception) {
        return status_code_to_response(404, server_config);
    }

    if (url_config.key_exists("redirect")) {
        return redirection_response(url_config["redirect"].unwrap());
    }

    std::vector<std::string> allowed_methods;
    {
        std::vector<Config> allowed_methods_cfg = url_config.get_vec("allow");
        for (Config& method_cfg : allowed_methods_cfg) {
            try {
                allowed_methods.push_back(method_cfg.unwrap());
            } catch (std::invalid_argument) {
                // ignore
            }
        }
    }
    std::string req_method = HttpRequest::method_to_str(request.get_method());
    if (std::all_of(allowed_methods.begin(), allowed_methods.end(), [&req_method](const std::string& method){return method != req_method;})) {
        return status_code_to_response(405, server_config);
    }

    std::string root; // TODO (or not): add default_root thingy
    try {
        root = url_config["root"].unwrap();
    } catch (std::exception) {
        std::cerr << "config error: no root\n";
        return status_code_to_response(500, server_config);
    }

    std::string server_url;
    try {
        server_url = url_config["url"].unwrap();
    } catch (...) {
        std::cerr << "how did we get here\n";
        exit(1);
    }

    std::string object_path = HttpHandle::compose_object_path(url, server_url, root);
    
    bool is_directory_listing = false;
    try {
        if (server_config["directory_listing"].unwrap() == "ON") {
            is_directory_listing = true;
        }
    } catch (std::exception) {
        is_directory_listing = false;
    }

    fs::path path(object_path);
    if (!fs::exists(path)) {
        return status_code_to_response(404, server_config);
    }
    if (fs::is_directory(path)) {
        if (is_directory_listing) {
            return directory_listing_response(object_path);
        }
        else {
            return no_directory_listing_response(object_path);
        }
    }

    if (!fs::is_regular_file(path)) {
        std::cerr << "Error: " + object_path + " is not dir or regular file\n";
        return status_code_to_response(500, server_config); // TODO: idk if 500
    }
    std::ifstream file(object_path);
    if (!file.good()) {
        std::cerr << "Error: couldn't read " << object_path << "\n";
        return status_code_to_response(500, server_config);
    }

    const std::string cgi_extension = ".py";
    if (path.extension() == cgi_extension) {
        return execute_cgi_response(object_path);
    }
    return file_response(std::move(file), path.extension());
}

std::string HttpHandle::file_response(std::ifstream&& file, const std::string& extension) {
    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string content = buffer.str();

    const std::string header = "HTTP/1.1 200 OK\n";
    std::string content_type = "text/plain";
    if (extension == ".html") {
        content_type = "text/html";
    } else if (extension == ".css") {
        content_type = "text/css";
    }

    content_type = "Content-Type: " + content_type + "\n";

    return header + content_type + "\n" + content;
}

std::string HttpHandle::redirection_response(const std::string& redirection_url) {
    return "HTTP/1.1 301 Moved Permanently\n" "location: " + redirection_url  + "\n";
}

std::string HttpHandle::directory_listing_response(const std::string& directory_path) {
    // TODO
    // might want to pass filesystem::path instead of string
    return "Directory listing for: " + directory_path;
}

std::string HttpHandle::no_directory_listing_response(const std::string& directory_path) {
    // TODO
    // might want to pass filesystem::path instead of string
    return "Index of directory for: " + directory_path;
}

std::string HttpHandle::execute_cgi_response(const std::string& script_path) {
    // TODO
    return "Executing cgi for: " + script_path;
}

Config HttpHandle::select_server_config(const HttpRequest& request, Config& config) {
    vector<Config> servers = config.get_vec("server");
    std::string request_host = request.get_host();
    int request_port = request.get_port();
    for (Config& server_config : servers) {
        int server_port;
        std::string server_host;
        try {
            std::string port_str = server_config["listen"].unwrap();
            server_port = std::stoi(port_str);
        } catch (std::invalid_argument) {
            continue;
        } catch (std::out_of_range) {
            server_port = 80; // default HTTP port
        }

        try {
            server_host = server_config["host"].unwrap();
        } catch (std::out_of_range) {
            continue;
        } catch (std::invalid_argument) {
            continue;
        }

        if (server_port == request_port && server_host == request_host) {
            return server_config;
        }
    }

    throw std::runtime_error("Didn't find matching config");
}

Config HttpHandle::select_url_config(const std::string& url, Config& server_config) {
    // iterate over urls, find url that is suburl of request url and the longest one among those
    std::vector<std::string> parsed_request_url = HttpRequest::parse_url(url);
    vector<Config> locations = server_config.get_vec("location");
    int max_prefix_size = -1;
    Config selected;
    for (Config& url_config : locations) {
        std::string location_url;
        try {
            location_url = url_config["url"].unwrap();
        } catch (std::exception) {
            continue;
        }
        vector<std::string> parsed_location_url = HttpRequest::parse_url(location_url);
        if (is_vector_prefix(parsed_location_url, parsed_request_url) == false) {
            continue;
        }
        if (static_cast<long long>(parsed_location_url.size()) > static_cast<long long>(max_prefix_size)) {
            selected = url_config;
            max_prefix_size = parsed_location_url.size();
            continue;
        }
    }
    if (max_prefix_size == -1) {
        throw std::runtime_error("Couldn't find matching location");
    }
    return selected;
}

std::string HttpHandle::compose_object_path(const std::string& url, const std::string& server_url, const std::string& root) {
    // replaces server_url to root in url
    std::vector<std::string> parsed_request_url = HttpRequest::parse_url(url);
    std::vector<std::string> parsed_server_url = HttpRequest::parse_url(server_url);
    std::vector<std::string> parsed_root = HttpRequest::parse_url(root);
    std::vector<std::string> result_path;
    result_path.insert(result_path.end(), parsed_root.begin(), parsed_root.end());
    result_path.insert(result_path.end(), parsed_request_url.begin() + parsed_server_url.size(), parsed_request_url.end());
    std::string joined_result = HttpRequest::join_url(result_path);
    if (trim(root)[0] == '/') {
        joined_result = "/" + joined_result;
    }
    return joined_result;
}

std::string HttpHandle::status_code_to_response(int status_code, Config& server_config) {
    const std::unordered_map<int, std::string> httpStatusCodes = {
        // 4xx Client Errors
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
        {406, "Not Acceptable"},
        {407, "Proxy Authentication Required"},
        {408, "Request Timeout"},
        {409, "Conflict"},
        {410, "Gone"},
        {411, "Length Required"},
        {413, "Payload Too Large"},
        {414, "URI Too Long"},
        {415, "Unsupported Media Type"},
        {416, "Range Not Satisfiable"},
        {417, "Expectation Failed"},
        {418, "I'm a teapot"},
        {421, "Misdirected Request"},
        {422, "Unprocessable Entity"},
        {423, "Locked"},
        {424, "Failed Dependency"},
        {425, "Too Early"},
        {426, "Upgrade Required"},
        {428, "Precondition Required"},
        {429, "Too Many Requests"},
        {431, "Request Header Fields Too Large"},
        {451, "Unavailable For Legal Reasons"},

        // 5xx Server Errors
        {500, "Internal Server Error"},
        {501, "Not Implemented"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
        {504, "Gateway Timeout"},
        {505, "HTTP Version Not Supported"},
        {506, "Variant Also Negotiates"},
        {507, "Insufficient Storage"},
        {508, "Loop Detected"},
        {510, "Not Extended"},
        {511, "Network Authentication Required"}
    };

    std::string error_name;
    auto error_it = httpStatusCodes.find(status_code);
    if (error_it == httpStatusCodes.end()) {
        error_name = "Unrecognized error";
    } else {
        error_name = error_it->second;
    }
    std::string error_message = std::to_string(status_code) + " " + error_name;
    
    std::string error_content = R"(<!DOCTYPE html>
<html>
  <head>
    <title>Error</title>
  </head>
  <body>
    <h1>An error occurred.</h1>
    <p>$ERROR$</p>
  </body>
</html>)"; // default
    try {
        const std::string error_file_path = server_config["errors"].unwrap();
        std::ifstream file(error_file_path);
        if (!file.good()) {throw std::exception();};
        std::stringstream buffer;
        buffer << file.rdbuf();
        error_content = buffer.str();
    } catch (std::exception) {
        // keep default content
    }
    const std::string ERROR_MARKER = "$ERROR$";
    error_content.replace(error_content.find(ERROR_MARKER), ERROR_MARKER.size(), error_message);

    return "HTTP/1.1 "
    + error_message + "\n"
    "Content-type: text/html\n"
    "\n"
    + error_content;
}
