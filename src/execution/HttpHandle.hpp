#pragma once

#include "HttpRequest.hpp"
#include "Config.hpp"

#include <fstream>
#include <variant>
#include <future>
#include <filesystem>

struct normalResponse {
  std::string response;
  bool is_keep_alive;
};

struct cgiResponse {
  int cgi_pid;
  int cgi_pipe[2];
  bool is_keep_alive;
};

struct requestNotFinished {
  bool is_chunked;
};

using response = std::variant<normalResponse, cgiResponse, requestNotFinished>;

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
    static response file_response(const std::string& file_path, Config& server_config, bool is_keep_alive);
    static response redirection_response(const std::string& redirection_url, bool is_keep_alive);
    static response directory_listing_response(const std::filesystem::path& directory_path, const std::string& url, bool is_keep_alive);
    static response no_directory_listing_response(const std::filesystem::path& directory_path, Config& url_config, Config& server_config, bool is_keep_alive);
    static std::string compose_object_path(const std::string& url, const std::string& server_url, const std::string& root);
    static response execute_cgi_response(std::string scipt_path, std::string arg, bool is_keep_alive);
    // static void run_cgi(std::promise<std::string>&& cgi_promise, const std::string& script_path, const std::string& arg); // TODO
    static response status_code_to_response(int status_code, Config& server_config, bool is_keep_alive);
    static response delete_file_response(const std::string& url_path, bool is_keep_alive);

    // defined in DirectoryListing.cpp
    static std::string directory_listing_html(const std::string& root_path, const std::vector<std::string>& leafs);
};
