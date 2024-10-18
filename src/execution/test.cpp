#include "Base.hpp"
#include "HttpHandle.hpp"

#include <iostream>

int main()
{
    std::string request = "POST /other_folder/a.html HTTP/1.1\r\nHost: not_the_best_host:8081\r\n";
    Config cfg("Config.cfg");
    auto resp = HttpHandle::compose_response(request, cfg);
    std::cout << std::get<std::string>(resp);
    // auto& future = std::get<std::future<std::string>>(resp);
    // while (!future_is_ready(future)) {
    //     std::cout << "still not ready\n";
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    // }
    // std::cout << future.get() << "\n";
}
