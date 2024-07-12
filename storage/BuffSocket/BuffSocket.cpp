// socket that send()s and recv()s only a chunk of the message
// it's BuffSocket::send() and BuffSocket::recv() send and read only a chunk
// These functions return 0 if they have sent or recv'd the whole message,
//  and 1 if they sucessfully sent or recv'd one chunk, but there's still some data left to send.
// also there should be methods IsAllDataRead() and IsAllDataSent()
//    (internally these methods can just check if the std::bufferRead/Write strings has been emptied, of I can create bool vars for it)
// Also there should be methods for retrieving the read data (exception of some indication of error if IsAllDataRead() is false)
// And the same for sending the data (AddDataTo"to_be_sent"Buffer() or seomthing like that)

#include "BuffSocket.hpp"
#include "ServerCore.hpp"

void BuffSocket::add_send_buff(const std::string &buff) {
  send_buffer += buff;
}

void BuffSocket::add_recv_buff(const std::string &buff) {
  recv_buffer += buff;
}

int BuffSocket::send_buff() {
  if (send_buffer.length() == 0) {
    return 0;
  }
  if (send_buffer.length() < buff_size) {
    return send(socket_fd, send_buffer.c_str(), send_buffer.length(), 0);
  }
  int ret_val = send(socket_fd, send_buffer.c_str(), buff_size, 0);
  if (ret_val < 0) {
    return ret_val;
  }
  send_buffer.erase(0, buff_size);
  return ret_val;
}

int BuffSocket::recv_buff() {
  char recv_temp_buff[buff_size + 1];
  int ret_val = recv(socket_fd, recv_temp_buff, buff_size, 0);
  recv_buffer += recv_temp_buff;
  /* 
  if there's EOF in recv_buffer, we've got the whole request, and now we immediately call request_handler
  request handler will determine if we need to use CGI
  and if we need to, it should somehow return pipe fd....
  whatever let's just ignore that CGI exists for now
  */
  return ret_val;
}

BuffSocket::BuffSocket() {}

BuffSocket::BuffSocket(const int &fd) {
  socket_fd = fd;
  buff_size = BUFF_SIZE_SERV;
}

BuffSocket::~BuffSocket() {}