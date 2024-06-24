#include "ServerCore.hpp"
#include "Config.hpp"
#include <iostream>

int main(int argc, char **argv){
  if (argc != 2) {
    exit(1);
  }
  Config config(argv[1]);
    std::cout << "host: " << config["server"]["host"].get_content() << std::endl;
    // std::cout << "host: " << config["database"]["host"].unwrap() << std::endl;
    // std::cout << "port: " << config["server"]["port"].unwrap() << std::endl;

  ServerCore core("127.0.0.1", 18000, 100);
  std::cout << "Server Core Created\n";
  core.setup();
  std::cout << "Server Core set up done\n";
  return(0);
}