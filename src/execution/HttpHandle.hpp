#pragma once

#include "HttpRequest.hpp"
#include "Config.hpp"

class HttpHandle {
  public:
    HttpHandle() = delete;
    static std::string make_response(const HttpRequest& request, const Config& config);
  private:
    
};
