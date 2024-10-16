#include "Base.hpp"
#include "HttpHandle.hpp"

#include <iostream>

int main()
{
    std::string request = "POST /other_folder/a.py HTTP/1.1\r\nHost: not_the_best_host:8081\r\n";
    Config cfg("Config.cfg");
    std::cout << HttpHandle::compose_response(request, cfg);
}
