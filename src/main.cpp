
#include "Config.hpp"
#include "ConnectionManager.hpp"
#include "Logger.hpp"
#include <iostream>
#include <map>
#include <string>
#include <vector>

int main(int argc, char **argv) {
  // (void)argc;
  // (void)argv;
  // Config config(argv[1]);
  // std::vector<Config> server_configs;
  // server_configs = config.get_vec("server");
  // for (size_t i = 0; i < server_configs.size(); i++) {
  //   std::cout << "Host: " << server_configs[i]["host"].unwrap() << " -> " << Config::string_to_ip(server_configs[i]["host"].unwrap()) << std::endl;
  // }
  // return 0;
  if (argc != 2) {
    std::cout << "gib args (conf file)\n";

    return -1;
  }
  Config config(argv[1]);
  Logger logger;
  ConnectionManager main_connection(&config, &logger);
  main_connection.setup();
  #ifdef DEBUG
  main_connection.print_connection_manager();
  #endif
  std::cout << "Running the webserv" << std::endl;
  if (main_connection.run() == -1) {
    std::cout << "Webserv died in a horrible way" << std::endl;
    return 1;
  }
  std::cout << "Webserv has been shut down correctly" << std::endl;
  return (0);
}
