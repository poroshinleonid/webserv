#ifndef AsyncFD_HPP
#define AsyncFD_HPP

#include <string>
#include <map>
#include <queue>

enum fd_type {
  FD_TYPE_FILE = 0,
  FD_TYPE_SOCK = 1,
  FD_TYPE_STRM = 2,
  FD_TYPE_PIPE_TO_READ = 3,
  FD_TYPE_PIPE_TO_WRIT = 4
};

class	AsyncFD {
public:
  AsyncFD();
  AsyncFD(AsyncFD const &other);
  ~AsyncFD();
  AsyncFD &operator=(const AsyncFD &obj);

  int async_write(const std::string &msg); //write or send determined by fd_type
  int async_read(std::string &buffer, int len);

  bool is_write_buf_empty();
  bool is_read_buf_empty();

private:
  int fd;
  fd_type type;
  std::string write_buff;
  std::string read_buff;

};

#endif
