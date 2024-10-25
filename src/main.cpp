
#include "Config.hpp"
#include "ConnectionManager.hpp"
#include "Logger.hpp"
#include <iostream>
#include <map>
#include <string>
#include <vector>

#define DEBUG

int main(int argc, char **argv) {
  setvbuf(stdout, NULL, _IONBF, 0); // cout no buffer
  if (argc != 2) {
    std::cout << "gib args (conf file)\n";

    return -1;
  }
  Config config(argv[1]);
  Logger logger;
  ConnectionManager main_connection(&config, &logger);
  main_connection.setup();
  main_connection.print_connection_manager();
  std::cout << "Running the webserv" << std::endl;
  if (main_connection.run() == -1) {
    std::cout << "Webserv died in a horrible way" << std::endl;
    return 1;
  }
  std::cout << "Webserv has been shut down correctly" << std::endl;
  return (0);
}
