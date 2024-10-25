#include "HttpHandle.hpp"
#include "Base.hpp"
#include "HttpRequest.hpp"
#include "Libft.hpp"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <future>
#include <thread>
#include <unistd.h>

std::string get_responses_string(HttpConnection &connection) {
  // TODO: (maybe) response to multiple requests
  std::string request_str = connection.recv_stream.str();
  std::cout << "Request to handle: [" << request_str << "]" << std::endl;
  Config config(*connection.config);
  HttpHandle::response response =
      HttpHandle::HttpHandle::compose_response(request_str, config);
  try {
    auto resp = std::get<HttpHandle::normalResponse>(response);
    connection.recv_stream.str(std::string());
    connection.is_response_ready = true;
    connection.is_keep_alive = resp.is_keep_alive;
    return resp.response;
  } catch (std::bad_variant_access &) { /*ignore*/
  }
  try {
    auto resp = std::get<HttpHandle::cgiResponse>(response);
    connection.recv_stream.str(std::string());
    connection.is_keep_alive = resp.is_keep_alive;
    connection.is_cgi_running = true;
    connection.cgi_finished = false;
    connection.cgi_pid = resp.cgi_pid;
    connection.cgi_pipe[0] = resp.cgi_pipe[0];
    connection.cgi_pipe[1] = resp.cgi_pipe[1];
    return "";
  } catch (std::bad_variant_access &) { /*ignore*/
  }
  try {
    auto resp = std::get<HttpHandle::requestNotFinished>(response);
    connection.is_chunked_transfer = resp.is_chunked_transfer;
    return "";
  } catch (std::bad_variant_access &) {
    std::cerr << "You wouldn't have such problem in rust\n";
    exit(1);
  }
}

namespace HttpHandle {
namespace fs = std::filesystem;

response HttpHandle::compose_response(const std::string &request_str,
                                      Config &config) {
  HttpRequest request;

  if (request_str.find("\r\n\r\n") == std::string::npos) {
    std::cout <<"REQUEST NOT FINISHED" << std::endl;
    return requestNotFinished{.is_chunked_transfer = true};
  }

  try {
    request = HttpRequest(request_str);
  } catch (HttpRequest::BadRequest &) {
    return status_code_to_response(400, config /*dummy*/, false /*default*/);
  } catch (HttpRequest::RequestNotFinished &) {
    return requestNotFinished{.is_chunked_transfer = true};
  }

  bool is_keep_alive = false;
  try {
    if (request.get_header_at("Connection") == "keep-alive") {
      is_keep_alive = true;
    }
  } catch (...) { /*ignore*/
  }

  if (request.get_body().size() > HttpRequest::MAX_BODY_SIZE) {
    return status_code_to_response(413, config /*dummy*/, is_keep_alive);
  }

  try {
    request.get_host();
    request.get_port();
  } catch (std::exception &) {
    return status_code_to_response(400, config /*dummy*/, is_keep_alive);
  }

  Config server_config;
  try {
    server_config = select_server_config(request, config);
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
    return status_code_to_response(404, config /*dummy*/, is_keep_alive);
  }

  std::string url = request.get_url();
  Config url_config;
  try {
    url_config = select_url_config(url, server_config);
  } catch (std::exception &) {
    std::cout << "404 2" << std::endl;
    return status_code_to_response(404, server_config, is_keep_alive);
  }

  if (url_config.key_exists("redirect")) {
    return redirection_response(url_config["redirect"].unwrap(), is_keep_alive);
  }

  std::vector<std::string> allowed_methods;
  {
    std::vector<Config> allowed_methods_cfg = url_config.get_vec("allow");
    for (Config &method_cfg : allowed_methods_cfg) {
      try {
        allowed_methods.push_back(method_cfg.unwrap());
      } catch (std::invalid_argument &) {
        // ignore
      }
    }
  }
  std::string req_method = HttpRequest::method_to_str(request.get_method());
  if (std::all_of(allowed_methods.begin(), allowed_methods.end(),
                  [&req_method](const std::string &method) {
                    return method != req_method;
                  })) {
    return status_code_to_response(405, server_config, is_keep_alive);
  }

  std::string root; // TODO (or not): add default_root thingy
  try {
    root = url_config["root"].unwrap();
  } catch (std::exception &) {
    std::cerr << "Config error: no root\n";
    return status_code_to_response(500, server_config, is_keep_alive);
  }

  std::string server_url;
  try {
    server_url = url_config["url"].unwrap();
  } catch (...) {
    std::cerr << "how did we get here\n";
    exit(1);
  }

  std::string object_path =
      HttpHandle::compose_object_path(url, server_url, root);

  bool is_directory_listing = false;
  try {
    if (server_config["autoindex"].unwrap() == "on") {
      is_directory_listing = true;
    }
  } catch (std::exception &) {
    is_directory_listing = false;
  }

  fs::path path(object_path);
  if (!fs::exists(path)) {
    std::cout << "404" << std::endl;
    return status_code_to_response(404, server_config, is_keep_alive);
  }
  if (fs::is_directory(path)) {
    if (request.get_method() == HttpRequest::Method::DELETE) {
      std::cerr << "Error: trying to DELETE a directory\n";
      return status_code_to_response(403, server_config, is_keep_alive);
    }
    if (is_directory_listing) {
      return directory_listing_response(path, url, is_keep_alive);
    } else {
      return no_directory_listing_response(path, url_config, server_config,
                                           is_keep_alive);
    }
  }

  if (!fs::is_regular_file(path)) {
    std::cerr << "Error: " + object_path + " is not dir or regular file\n";
    return status_code_to_response(500, server_config,
                                   is_keep_alive); // TODO: idk if 500
  }

  if (request.get_method() == HttpRequest::Method::DELETE) {
    bool was_removed = fs::remove(path);
    if (!was_removed) {
      std::cerr << "Error: couldn't remove " + object_path + "\n";
      return status_code_to_response(500, server_config, is_keep_alive);
    }
    return delete_file_response(url, is_keep_alive);
  }

  std::ifstream file(object_path);
  if (!file.good()) {
    std::cerr << "Error: couldn't read " << object_path << "\n";
    return status_code_to_response(404, server_config, is_keep_alive);
  }
  const std::string cgi_extension = ".py";
  try {
    if (path.extension() == cgi_extension) {
      if (request.get_method() == HttpRequest::Method::POST) {
        return execute_cgi_response(object_path, request.get_body(),
                                    is_keep_alive);
      } else {
        return execute_cgi_response(object_path, "", is_keep_alive);
      }
    }
  } catch (std::runtime_error &e) {
    std::cerr << "[ERROR] CGI failed to fork properly" << std::endl;
    return status_code_to_response(500, server_config, is_keep_alive);
  }
  return file_response(object_path, server_config, is_keep_alive);
}

std::string HttpHandle::ok_response_head(ContentType t) {
  std::string content_name;
  switch (t) {
  case ContentType::plain:
    content_name = "text/plain";
    break;
  case ContentType::html:
    content_name = "text/html";
    break;
  case ContentType::css:
    content_name = "text/css";
    break;
  }
  std::string response_head =
      "HTTP/1.1 200 OK\r\nContent-type: " + content_name + "\r\n\r\n";
  return response_head;
}

response HttpHandle::file_response(const std::string &file_path,
                                   Config &server_config, bool is_keep_alive) {
  std::ifstream file(file_path);
  if (!file.good()) {
    std::cerr << "Error: couldn't read " << file_path << "\n";
    return status_code_to_response(404, server_config, is_keep_alive);
  }
  std::stringstream buffer;
  buffer << file.rdbuf();

  std::string content = buffer.str();

  ContentType content_type;
  std::string extension = fs::path(file_path).extension();
  if (extension == ".html") {
    content_type = ContentType::html;
  } else if (extension == ".css") {
    content_type = ContentType::css;
  } else {
    content_type = ContentType::plain;
  }

  std::string response_header_str = ok_response_head(content_type);
  if (response_header_str.length() >= 2) {
    response_header_str.resize(response_header_str.length() - 2);
  }
  response_header_str.append(
      "Content-Length: " + Libft::ft_itos(content.size()) + "\r\n\r\n");

  return normalResponse{.response = response_header_str + content,
                        .is_keep_alive = is_keep_alive};
}

response HttpHandle::redirection_response(const std::string &redirection_url,
                                          bool is_keep_alive) {
  return normalResponse{.response = "HTTP/1.1 301 Moved Permanently\r\n"
                                    "location: " +
                                    redirection_url + "\r\n\r\n",
                        .is_keep_alive = is_keep_alive};
}

response HttpHandle::directory_listing_response(const fs::path &directory_path,
                                                const std::string &url,
                                                bool is_keep_alive) {
  std::vector<std::string> leafs;
  // TODO: maybe ..
  for (auto const &dir_entry : fs::directory_iterator(directory_path)) {
    leafs.push_back(dir_entry.path().filename().string());
  }
  std::string html_content = directory_listing_html(url, leafs);
  std::string response_header_str = ok_response_head(ContentType::html);
  if (response_header_str.length() >= 2) {
    response_header_str.resize(response_header_str.length() - 2);
  }
  response_header_str.append(
      "Content-Length: " + Libft::ft_itos(html_content.size()) + "\r\n\r\n");

  return normalResponse{.response = response_header_str + html_content,
                        .is_keep_alive = is_keep_alive};
}

response HttpHandle::no_directory_listing_response(
    const fs::path &directory_path, Config &url_config, Config &server_config,
    bool is_keep_alive) {
  fs::path index = "index.html";
  try {
    index = url_config["index"].unwrap();
  } catch (...) {
    // ignore
  }
  index = directory_path / index;
  return file_response(index, server_config, is_keep_alive);
}

response HttpHandle::execute_cgi_response(const std::string &script_path,
                                          const std::string &arg,
                                          bool is_keep_alive) {
  int fd[2];
  if (pipe(fd) == -1) {
    std::cerr << "Pipe error\n";
    throw std::runtime_error("Pipe error");
  }
  int pid_t = fork();
  if (pid_t == -1) {
    throw std::runtime_error("Fork error");
  }
  if (pid_t == 0) {
    close(fd[0]);
    if (dup2(fd[1], STDOUT_FILENO) == -1) {
      std::cerr << "dup2 error\n";
      close(fd[1]);
      exit(1);
    }
    close(fd[1]);
    char *const argv[] = {const_cast<char *>(script_path.c_str()),
                          const_cast<char *>(arg.c_str()), nullptr};
    sleep(1);
    if (execve(script_path.c_str(), argv, NULL) ==
        -1) { // TODO: how to write the whole response instead of script output
      std::cerr << "Error executing cgi\n";
      exit(1);
    }
    exit(1);
  }
  close(fd[1]);
  cgiResponse res;
  res.cgi_pid = pid_t;
  res.is_keep_alive = is_keep_alive;
  res.cgi_pipe[0] = fd[0];
  res.cgi_pipe[1] = fd[1];
  return res;
}

Config HttpHandle::select_server_config(const HttpRequest &request,
                                        Config &config) {
  vector<Config> servers = config.get_vec("server");
  std::string request_host = request.get_host();
  int request_port = request.get_port();
  for (Config &server_config : servers) {
    int server_port;
    std::string server_host;
    try {
      std::string port_str = server_config["listen"].unwrap();
      server_port = std::stoi(port_str);
    } catch (std::invalid_argument &) {
      continue;
    } catch (std::out_of_range &) {
      server_port = 80; // default HTTP port
    }

    try {
      server_host = server_config["host"].unwrap();
    } catch (std::out_of_range &) {
      continue;
    } catch (std::invalid_argument &) {
      continue;
    }

    if (server_port == request_port && server_host == request_host) {
      return server_config;
    }
  }

  throw std::runtime_error("Didn't find matching config");
}

Config HttpHandle::select_url_config(const std::string &url,
                                     Config &server_config) {
  // iterate over urls, find url that is suburl of request url and the longest
  // one among those
  std::vector<std::string> parsed_request_url = HttpRequest::parse_url(url);
  vector<Config> locations = server_config.get_vec("location");
  int max_prefix_size = -1;
  Config selected;
  for (Config &url_config : locations) {
    std::string location_url;
    try {
      location_url = url_config["url"].unwrap();
    } catch (std::exception &) {
      continue;
    }
    vector<std::string> parsed_location_url =
        HttpRequest::parse_url(location_url);
    if (is_vector_prefix(parsed_location_url, parsed_request_url) == false) {
      continue;
    }
    if (static_cast<long long>(parsed_location_url.size()) >
        static_cast<long long>(max_prefix_size)) {
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

// void printve(const std::string &name, std::vector<std::string> &v) {
//   std::cout << name << ": ";
//   for (auto it = v.begin(); it != v.end(); ++it){
//     std::cout << *it << " ";
//   }
//   std::cout << std::endl;
// }

std::string HttpHandle::compose_object_path(const std::string &url,
                                            const std::string &server_url,
                                            const std::string &root) {
  // replaces server_url to root in url
  std::vector<std::string> parsed_request_url = HttpRequest::parse_url(url);
  std::vector<std::string> parsed_server_url =
      HttpRequest::parse_url(server_url);
  std::vector<std::string> parsed_root = HttpRequest::parse_url(root);
  std::vector<std::string> result_path;
  result_path.insert(result_path.end(), parsed_root.begin(), parsed_root.end());
  result_path.insert(result_path.end(),
                     parsed_request_url.begin() + parsed_server_url.size(),
                     parsed_request_url.end());
  std::string joined_result = HttpRequest::join_url(result_path);
  if (trim(root)[0] == '/') {
    joined_result = "/" + joined_result;
  }
  // std::cout << "path: " << joined_result << std::endl;
  return joined_result;
}

response HttpHandle::status_code_to_response(int status_code,
                                             Config &server_config,
                                             bool is_keep_alive) {
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
      {511, "Network Authentication Required"}};

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
    if (!file.good()) {
      throw std::exception();
    };
    std::stringstream buffer;
    buffer << file.rdbuf();
    error_content = buffer.str();
  } catch (std::exception &) {
    // keep default content
  }
  const std::string ERROR_MARKER = "$ERROR$";
  error_content.replace(error_content.find(ERROR_MARKER), ERROR_MARKER.size(),
                        error_message);

  std::string content_sz_str =
      "Content-Length: " + Libft::ft_itos(error_content.size()) + "\r\n";

  return normalResponse{.response = "HTTP/1.1 " + error_message +
                                    "\r\n"
                                    "Content-type: text/html\r\n" +
                                    content_sz_str + "\r\n" + error_content,
                        .is_keep_alive = is_keep_alive};
}

response HttpHandle::delete_file_response(const std::string &url_path,
                                          bool is_keep_alive) {
  std::string message = "sucesfully deleted: " + url_path + '\n';
  return normalResponse{.response =
                            ok_response_head(ContentType::plain) + message,
                        .is_keep_alive = is_keep_alive};
}
}; // namespace HttpHandle
