#include "Config.hpp"
#include "Logger.hpp"
#include "ConnectionManager.hpp"
#include <iostream>

int main(int argc, char **argv){
  if (argc != 2) {
  std::cout << "gib args (conf file)\n";

    return -1;
  }
  Config config(argv[1]);
  Logger logger;
  ConnectionManager main_connection(&config, &logger);
  main_connection.setup();
  main_connection.run();

  return(0);
}