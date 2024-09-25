#pragma once

#include "HttpRequest.hpp"
#include "Config.hpp"

class HttpHandle {
  public:
    HttpHandle() = delete;
    static std::string compose_response(const std::string& request_str, Config& config);
  private:
    static std::string status_code_to_response(int status_code);
    static Config select_server_config(const HttpRequest& request, Config& config); // determines server config based on port and host. throws -> 404
    static Config select_url_config(const std::string& url, Config& server_config); // selects config matching the url. throws -> 404
    static void is_url_redirection(const HttpRequest& request, Config& config);
    static void check_if_method_is_allowed(const HttpRequest& request, Config& config);

    // static void 
    /*
    0. parse http and make sure its valid, status code 400 if not (maybe accept std::string as request for that purpose)
    0.1. check content length
    1. determine which server from port and host
    2. parse URL and cd in the right file
      2.1. check if URL is redirection
      2.2. check if request method is allowed
      2.2.1. check content length (should be 0 on GET, and < max_content_length on POST)
      2.3. if directory -> directory listing / default file
      2.4. check if the file extension is for cgi
      ^ those might be in a different order ^
    */
};
