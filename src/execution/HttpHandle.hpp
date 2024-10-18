#pragma once

#include "HttpRequest.hpp"
#include "Config.hpp"

#include <fstream>
#include <variant>
#include <future>
#include <filesystem>

using response = std::variant<std::string, std::future<std::string>>;

class HttpHandle {
  public:
    HttpHandle() = delete;
    static response compose_response(const std::string& request_str, Config& config);
  private:
    static Config select_server_config(const HttpRequest& request, Config& config); // determines server config based on port and host. throws -> 404
    static Config select_url_config(const std::string& url, Config& server_config); // selects config matching the url. throws -> 404

    enum class ContentType {
      plain,
      html,
      css,
    };
    static std::string ok_response_head(ContentType t);
    static std::string file_response(const std::string& file_path, Config& server_config);
    static std::string redirection_response(const std::string& redirection_url);
    static std::string directory_listing_response(const std::string& directory_path);
    static std::string no_directory_listing_response(const std::filesystem::path& directory_path, Config& url_config, Config& server_config);
    static std::string compose_object_path(const std::string& url, const std::string& server_url, const std::string& root);
    static std::future<std::string> execute_cgi_response(const std::string& scipt_path);
    static void run_cgi(std::promise<std::string>&& cgi_promise, const std::string& script_path);
    static std::string status_code_to_response(int status_code, Config& server_config);
    // static void 
    /*
    0. parse http and make sure its valid, status code 400 if not (maybe accept std::string as request for that purpose)
    0.1. check content length
    1. determine which server from port and host
    2. parse URL and cd in the right file
      2.1. check if URL is redirection
      2.2. check if request method is allowed
      2.3. if directory -> directory listing / default file
      2.4. check if the file extension is for cgi
      ^ those might be in a different order ^
    */
};
