
#include "Config.hpp"
#include "ConnectionManager.hpp"
#include "Logger.hpp"
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "signal.h"

int main(int argc, char **argv) {
  signal(SIGCHLD, SIG_IGN);
  if (argc != 2) {
    std::cout << "gib args (conf file)\n";

    return -1;
  }
  Config config(argv[1]);
  Logger logger;
  #ifdef DEBUG
    logger.log_debug("test");
    logger.log_info("test");
    logger.log_warning("test");
    logger.log_error("test");
    std::cout << "test" << std::endl;
  #endif
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
