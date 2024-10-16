#include "HttpHandle.hpp"
#include "HttpRequest.hpp"
#include "Base.hpp"

#include <algorithm>

std::string HttpHandle::compose_response(const std::string& request_str, Config& config) {
    HttpRequest request;

    try {
        request = HttpRequest(request_str);
    } catch (HttpRequest::BadRequest) {
        return status_code_to_response(400);
    }

    if (request.get_body().size() > HttpRequest::MAX_BODY_SIZE) {
        return status_code_to_response(413);
    }

    try {
        request.get_host();
        request.get_port();
    } catch (std::exception) {
        return status_code_to_response(400);
    }

    Config server_config;
    try {
        server_config = select_server_config(request, config);
    } catch (std::exception) {
        return status_code_to_response(404);
    }

    std::string url = request.get_url();
    Config url_config;
    try {
        url_config = select_url_config(url, server_config);        
    } catch (std::exception) {
        return status_code_to_response(404);
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
        return status_code_to_response(405);
    }

    return "all ok";
}

std::string HttpHandle::status_code_to_response(int status_code) {
    // TODO
    return "Oh no!" + std::to_string(status_code);
}

std::string HttpHandle::redirection_response(const std::string& redirection_url) {
    return "HTTP/1.1 301 Moved Permanently\n" "location: " + redirection_url  + "\n";
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
