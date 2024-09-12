
#include "Config.hpp"
#include "ConnectionManager.hpp"
#include "Logger.hpp"
#include <iostream>
#include <map>
#include <string>
#include <vector>


#define DEBUG

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "gib args (conf file)\n";

    return -1;
  }
  Config config(argv[1]);
  std::vector<Config> v;
  std::map<std::string, std::string> dict;
  std::map<std::string, std::string>::iterator it;

  // v = config["dictionaries"].get_vec("dict");
  // for (size_t i = 0; i < v.size(); i++) {
  //   std::cout << "DICT ITSELF: ";
  //   std::cout << v[i].get_content() << std::endl;
  //   dict = v[i].get_content_dict();
  //   std::cout << "DICT CONTENT" << std::endl;
  //   for (it = dict.begin(); it != dict.end(); it++) {
  //     std::cout << it->first << ":" << it->second << ";" << std::endl;
  //   }
  //   std::cout << "----------" << std::endl;
  // }
  // return 0;
  Logger logger;
  ConnectionManager main_connection(&config, &logger);
  main_connection.setup();
  main_connection.print_connection_manager();
  std::cout << "Running server" << std::endl;
  if (main_connection.run() == -1) {
    std::cout << "server died in a horrible way";
    return 1;
  }
  return (0);
}
