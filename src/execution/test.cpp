#include "HttpParse.hpp"
#include "Base.hpp"

#include <iostream>

int main() {
    std::string httpRequest = 
    "POST   aasdfadsf.asdg/.    HTTP/1.0\r\n"
    "Host: www.example.com\r\n"
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
    "Accept-Language: en-US,en;q=0.5\r\n"
    "Accept-Encoding: gzip, deflate\r\n"
    "Connection: keep-alive\r\n"
    "\r\n" // Blank line to indicate the end of the headers
    "abc\r\n"
    "def\r\n";
    std::stringstream s(httpRequest);
    HttpRequest req(s);
    std::cout << req.get_header_at("Accept ") << '\n';
}
