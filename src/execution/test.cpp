#include "Base.hpp"
#include "HttpHandle.hpp"

#include <iostream>
#include <unistd.h>

// Raw compose_response test
// int main()
// {
//     std::string request = "DELETE /server_one/uploads/a HTTP/1.1\r\nHost: not_the_best_host:8081\r\n\r\narg1 arg2\r\narg3\r\n";
//     Config cfg("Config.cfg");
//     HttpHandle::response resp = HttpHandle::HttpHandle::compose_response(request, cfg);
//     // auto re = std::get<cgiResponse>(resp);
//     // while (true) {
//     //     char buff[1000];
//     //     std::cout << "waiting\n";
//     //     if (read(re.cgi_pipe[0], buff, 100) > 1) {
//     //         std::cout << buff << '\n';
//     //     }
//     //     sleep(1);
//     // }
//     auto re = std::get<HttpHandle::normalResponse>(resp);
//     std::cout << re.response << '\n';
// }

int main() {
    HttpConnection connection;
    connection.is_response_ready = 0; connection.is_keep_alive = 0; connection.is_chunked_transfer = 0;
    connection.is_cgi_running = 0; connection.cgi_pid = 0; connection.cgi_pipe[0] = 0; connection.cgi_pipe[1] = 0;
    std::string request = "POST /server_one/uploads/upload_cgi.py HTTP/1.1\r\nHost: not_the_best_host:8081\r\nConnection: Keep-Alive\r\n\r\narg1 arg2\r\narg3\r\n";
    connection.recv_stream << request;
    connection.config = new Config("Config.cfg");
    std::string response = get_responses_string(connection);
    std::cout << response << "\nrecv_stream: " << connection.recv_stream.str() <<
    "\nis_response_ready: " << connection.is_response_ready <<
    "\nis_keep_alive: " << connection.is_keep_alive <<
    "\nis_chunked_tranfer: " << connection.is_chunked_transfer << 
    "\nis_cgi_running: " << connection.is_cgi_running <<
    "\ncgi_pid: " << connection.cgi_pid <<
    "\ncgi_pipe: " << connection.cgi_pipe[0] << ' ' << connection.cgi_pipe[1] << '\n';
    while (true) {}
}
