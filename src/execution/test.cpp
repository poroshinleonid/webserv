#include "Base.hpp"
#include "HttpHandle.hpp"

#include <iostream>

int main()
{
    std::string request = "POST /server_one/uploads/upload_cgi.py HTTP/1.1\r\nHost: not_the_best_host:8081\r\n\r\narg1 arg2\r\narg3\r\n";
    Config cfg("Config.cfg");
    auto resp = HttpHandle::compose_response(request, cfg);
    // std::cout << std::get<std::string>(resp) << '\n';
    auto& future = std::get<std::future<std::string>>(resp);
    while (!future_is_ready(future)) {
        std::cout << "still not ready\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << future.get() << "\n";
}
