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
#include <cerrno>

bool check_chunked_transfer(const std::string &request_str) {
  if (request_str.find("transfer-encoding: chunked\r\n") != std::string::npos && \
      request_str.find(CRLFCRLF) != std::string::npos) {
    return true;
  }
  return false;
}

bool is_oversized(const std::string &s) {
  size_t pos = s.find(CRLFCRLF);
  if (pos == std::string::npos) {
    pos = 0;
  }
  if (s.size() - pos > HttpRequest::MAX_BODY_SIZE) {
    return true;
  }
  return false;
}

namespace HttpHandle {
namespace fs = std::filesystem;

response HttpHandle::compose_response(const std::string &request_str,
                                      Config &config, HttpConnection &connection) {
  Logger &log = *connection.logger;
  HttpRequest request;
  try {
    request = HttpRequest(request_str);
  } catch(HttpRequest::UriTooLong &e) {
    log.log_info("URI to long: " + std::string(e.what()));
    return status_code_to_response(414, config /*dummy*/, false /*default*/);
  } catch (HttpRequest::BadRequest &e) {
    log.log_info("Bad Request: " + std::string(e.what()));
    return status_code_to_response(400, config /*dummy*/, false /*default*/);
  } catch (HttpRequest::RequestNotFinished &e) {
    if (std::string(e.what()) == "not chunked") {
      return requestNotFinished{.is_chunked_transfer = false};
    } else {
      return requestNotFinished{.is_chunked_transfer = true};
    }
  }

  try {
    if (std::stoi(request.get_header_at("content-length")) > connection.serv->client_max_body_size
    || request.get_body().size() > static_cast<size_t>(connection.serv->client_max_body_size)) {
      return status_code_to_response(413, config /* dummy */, false);
    }
  } catch (...) { /* ignore */}

  bool is_keep_alive = false;
  try {
    if (request.get_header_at("connection") == "keep-alive") {
      is_keep_alive = true;
    }
  } catch (...) { /*ignore*/
  }

  try {
    request.get_host();
    request.get_port();
  } catch (std::exception &e) {
    return status_code_to_response(400, config /*dummy*/, is_keep_alive);
  }

  Config server_config;
  try {
    server_config = select_server_config(request, config);
  } catch (std::exception &e) {
    log.log_warning(std::string("Select server config error: ") + std::string(e.what()));
    return status_code_to_response(404, config /*dummy*/, is_keep_alive);
  }

  std::string url = request.get_url();
  Config url_config;
  try {
    url_config = select_url_config(url, server_config);
  } catch (std::exception &) {
    return status_code_to_response(404, server_config, is_keep_alive);
  }

  if (url_config.key_exists("redirect")) {
    auto rsp = std::get<normalResponse>(redirection_response(url_config["redirect"].unwrap(), is_keep_alive));
    rsp.request_str_len = request.get_body().size() + request.get_header().size();
    rsp.request_str_len += 4; // CRLFCRLF
    return rsp;
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
    log.log_error("Config error: no root");
    return status_code_to_response(500, server_config, is_keep_alive);
  }

  std::string server_url;
  try {
    server_url = url_config["url"].unwrap();
  } catch (...) {
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
    return status_code_to_response(404, server_config, is_keep_alive);
  }
  if (fs::is_directory(path)) {
    if (request.get_method() == HttpRequest::Method::DELETE) {
      log.log_warning("trying to DELETE a directory");
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
    log.log_warning(object_path + " is not a dir or regular file");
    return status_code_to_response(500, server_config,
                                   is_keep_alive); // TODO: idk if 500
  }

  if (request.get_method() == HttpRequest::Method::DELETE) {
    bool was_removed = fs::remove(path);
    if (!was_removed) {
    log.log_warning("Couldn't remove " + object_path);
      return status_code_to_response(500, server_config, is_keep_alive);
    }
    return delete_file_response(url, is_keep_alive);
  }

  std::ifstream file(object_path);
  if (!file.good()) {
    if (errno == EACCES) {
      return status_code_to_response(403, server_config, is_keep_alive);
    } else if (errno == ENOENT) {
      return status_code_to_response(404, server_config, is_keep_alive);
    } else {
    log.log_warning("Something wrong with file: " + object_path + ", errno: " + Libft::ft_itos(errno));
      return status_code_to_response(500, server_config, is_keep_alive);
    }
  }
  // const std::string cgi_extension = ".py";
  const std::string cgi_extension = ".py";
  try {
    if (path.extension() == cgi_extension) {
      response resp;
      log.log_debug("Sending to CGI: " + request.get_body());
      connection.cgi_write_buffer = request.get_body();
      if (request.get_method() == HttpRequest::Method::POST) {
        // here we assume we have read all content-length
        #if 0
        std::string &body = connection.cgi_write_buffer;
        std::string content_type_str;
        try {
          content_type_str = request.get_header_at("Content-Type:");
          if (content_type_str.find("multipart/form-data") != std::string::npos) {
            std::string boundary_signifier = "boundary=";
            size_t boundary_str_start = content_type_str.find(boundary_signifier) + boundary_signifier.size();
            size_t boundary_str_end = content_type_str.find(CRLF, boundary_str_start);
            size_t boundary_str_sz = boundary_str_end - boundary_str_start;
            size_t boundary_str = content_type_str.substr(boundary_str_start, boundary_str_sz);
            
            size_t boundary_pos = body.find(boundary_str);
            if (boundary_pos != std::string::npos) {
              size_t metadata_pos = boundary_pos + boundary_str.size() + 2;
              //parse metadata to determine the name of the image
          // we find boundary, remove it and all of the content from it an up to CRLF
          // the rest of the string is a byte sequence that
          // is an image
            }
          }
        } catch (...) { /*ignore*/ }
        #endif
        resp = execute_cgi_response(object_path, request,
                                    is_keep_alive);
        return resp;
      } else {
        resp = execute_cgi_response(object_path, request,
                                    is_keep_alive); // execute_cgi_response(object_path, "", is_keep_alive);
        connection.cgi_write_buffer = "";
        return resp;
      }
    }
  } catch (std::runtime_error &e) {
    log.log_error("CGI failed to fork properly");
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
    if (errno == EACCES) {
      return status_code_to_response(403, server_config, is_keep_alive);
    } else if (errno == ENOENT) {
      return status_code_to_response(404, server_config, is_keep_alive);
    } else {
      std::cerr << "[ERROR] Something wrong with file:" << file_path << ", errno: " << errno << std::endl;
      return status_code_to_response(500, server_config, is_keep_alive);
    }
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
                        .is_keep_alive = is_keep_alive, .request_str_len = 0};
}

response HttpHandle::redirection_response(const std::string &redirection_url,
                                          bool is_keep_alive) {
  (void) is_keep_alive;
  return normalResponse{.response = "HTTP/1.1 302 Found\r\n"
                                    "Location: " +
                                    redirection_url + "\r\n\r\n",
                        .is_keep_alive = false, .request_str_len = 0};
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
                        .is_keep_alive = is_keep_alive, .request_str_len = 0};
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
                                          HttpRequest &request,
                                          bool is_keep_alive) {
  std::string arg = request.get_body();
  int recv_pipe[2];
  int send_pipe[2];
  if (pipe(recv_pipe) == -1) {
    std::cerr << "Pipe error\n";
    throw std::runtime_error("Pipe error");
  }
  if (pipe(send_pipe) == -1) {
    std::cerr << "Pipe error\n";
    throw std::runtime_error("Pipe error");
  }
  int pid_t = fork();
  if (pid_t == -1) {
    close(recv_pipe[0]);
    close(recv_pipe[1]);
    close(send_pipe[0]);
    close(send_pipe[1]);
    throw std::runtime_error("Fork error");
  } else if (pid_t == 0) { // we are inside the fork
    if (dup2(recv_pipe[1], STDOUT_FILENO) == -1) {
      std::cerr << "dup2 error\n";
      close(recv_pipe[1]);
      exit(1);
    }
    if (dup2(send_pipe[0], STDIN_FILENO) == -1) {
      std::cerr << "dup2 error\n";
      close(send_pipe[0]);
      exit(1);
    }
    close(recv_pipe[0]);
    close(recv_pipe[1]);
    close(send_pipe[0]);
    close(send_pipe[1]);
    std::string resp_s = request.get_response_str();
    const char *resp_c_str = resp_s.c_str();
    const std::string CGI_PATH = "/usr/bin/python3";
    char *const argv[] = { const_cast<char *>(CGI_PATH.c_str()),
                          const_cast<char *>(script_path.c_str()),
                          const_cast<char *>(resp_c_str), NULL};

    auto headers = request.get_header_map();
    std::vector<const char*> envp;
    for (auto it = headers.begin(); it != headers.end(); it++) {
      std::string var_name;
      std::string var_value;
      std::transform(it->first.begin(), it->first.end(), std::back_inserter(var_name), Libft::toenv);
      std::transform(it->second.begin(), it->second.end(), std::back_inserter(var_value), Libft::toenv);
      std::string var_decl = var_name + "="  + var_value;
      envp.push_back(std::move(var_decl.c_str()));
    }
    std::string tmp_s = "REQUEST_METHOD=" + HttpRequest::method_to_str(request.get_method());
    envp.push_back(std::move(tmp_s.c_str()));
    std::string tmp_s2 = "SERVER_PROTOCOL=HTTP/1.1";
    envp.push_back(std::move(tmp_s2.c_str()));
    std::string tmp_s3 = "PATH_INFO=" + request.get_url();
    envp.push_back(std::move(tmp_s3.c_str()));
    envp.push_back(NULL);
    if (execve(CGI_PATH.c_str(), const_cast<char* const*>(argv), const_cast<char* const*>(envp.data())) ==
        -1) {
      status_code_to_string(500);
      exit(1);
    }
    exit(1);
  }
  // pid != 0 - we are inside the parent
  close(send_pipe[0]);
  close(recv_pipe[1]);
  #ifdef DEBUG
  std::cout << "PIPES CREATED IN HANDLE: read=" << recv_pipe[0] << ", write=" << send_pipe[1] << ", pid=" << pid_t << std::endl;
  #endif
  cgiResponse res;
  res.request_str_len = request.get_body().size() + request.get_header().size();
  res.request_str_len += 4; // CRLFCRLF
  res.cgi_pid = pid_t;
  res.is_keep_alive = is_keep_alive;
  res.cgi_pipe[0] = recv_pipe[0];
  res.cgi_pipe[1] = send_pipe[1];
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
                        .is_keep_alive = is_keep_alive, .request_str_len = 0};
}

response HttpHandle::delete_file_response(const std::string &url_path,
                                          bool is_keep_alive) {
  std::string message = "sucesfully deleted: " + url_path + '\n';
  return normalResponse{.response =
                            ok_response_head(ContentType::plain) + message,
                        .is_keep_alive = is_keep_alive, .request_str_len = 0};
}
}; // namespace HttpHandle

std::string status_code_to_string(int status_code) {
  Config dummy;
  return std::get<HttpHandle::normalResponse>(HttpHandle::HttpHandle::status_code_to_response(status_code, dummy, false)).response;
}

// TODO: (maybe) response to multiple requests
std::string get_responses_string(HttpConnection &connection) {
  Config config(*connection.config);
  std::string request_str = connection.recv_buffer;
  HttpHandle::response response = HttpHandle::HttpHandle::compose_response(request_str, config, connection);
  try {
    auto resp = std::get<HttpHandle::normalResponse>(response);
    // not always clear(), remove header.size() + body.size() (also account for CRLFCRLF if we trimmed it)
    #ifdef DEBUG
    std::cout << "request_str: [" << connection.recv_buffer << "]" << std::endl;
    #endif
    //here and lower:
    //remover only response_str_len characters
    //then check if we had trailing CRLFCRLF and remove them also
    if (resp.request_str_len > 0) {
      connection.recv_buffer.erase(0, resp.request_str_len);
      while (connection.recv_buffer.find(CRLF) == 0) {
        connection.recv_buffer.erase(0, 2);
      }
    } else {
      connection.recv_buffer.clear();
    }
    connection.is_response_ready = true;
    connection.is_keep_alive = resp.is_keep_alive;
    return resp.response;
  } catch (std::bad_variant_access &) { /*ignore*/
  }
  try {
    #ifdef DEBUG
    std::cout << "request_str: [" << connection.recv_buffer << "]" << std::endl;
    #endif
    auto resp = std::get<HttpHandle::cgiResponse>(response);
    //here and higher:
    //remover only response_str_len characters
    //then check if we had trailing CRLFCRLF and remove them also
    if (resp.request_str_len > 0) {
      connection.recv_buffer.erase(0, resp.request_str_len);
      while (connection.recv_buffer.find(CRLF) == 0) {
        connection.recv_buffer.erase(0, 2);
      }
    } else {
      connection.recv_buffer.clear();
    }
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
    return "";
  }
}