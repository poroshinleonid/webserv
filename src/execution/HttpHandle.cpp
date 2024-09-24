#include "HttpHandle.hpp"
#include "HttpRequest.hpp"

std::string HttpHandle::compose_response(const std::string& request_str, const Config& config) {
    HttpRequest request;
    try {
        request = HttpRequest(request_str);
    } catch (HttpRequest::BadRequest) {
        return status_code_to_respose(400);
    }

    Config server_config;
    try {
        server_config = select_server_config(request, config);
    } catch (std::exception) {
        return status_code_to_respose(413);
    }
    
}
