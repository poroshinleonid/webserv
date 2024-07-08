#ifndef AsyncFD_HPP
#define AsyncFD_HPP

#include <string>

enum fd_type {
  file = 0,
  socket = 1,
  stream = 2,
  pipe_to_read = 3,
  pipe_to_write = 4
};

class	AsyncFD {
public:
  AsyncFD();
  AsyncFD(AsyncFD const &other);
  ~AsyncFD();
  AsyncFD &operator=(const AsyncFD &obj);

  int async_write(const std::string &msg);
  int async_read(std::string &buffer, int len);

private:
  int fd;
  fd_type type;
  std::string write_buff;
  std::string read_buff;

};

#endif
