#include "Base.hpp"
#include "HttpHandle.hpp"

#include <iostream>
#include <unistd.h>

int main()
{
    std::string request = "DELETE /server_one/uploads/a HTTP/1.1\r\nHost: not_the_best_host:8081\r\n\r\narg1 arg2\r\narg3\r\n";
    Config cfg("Config.cfg");
    response resp = HttpHandle::compose_response(request, cfg);
    // auto re = std::get<cgiResponse>(resp);
    // while (true) {
    //     char buff[1000];
    //     std::cout << "waiting\n";
    //     if (read(re.cgi_pipe[0], buff, 100) > 1) {
    //         std::cout << buff << '\n';
    //     }
    //     sleep(1);
    // }
    auto re = std::get<normalResponse>(resp);
    std::cout << re.response << '\n';
}
