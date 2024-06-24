#include "ServerSocket.hpp"
#include "Config.hpp"

int main(int argc, char **argv){
  if (argc != 2) {
    exit(1);
  }
  Config config(argv[1]);
  
}