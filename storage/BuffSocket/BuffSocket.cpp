// socket that send()s and recv()s only a chunk of the message
// it's BuffSocket::send() and BuffSocket::recv() send and read only a chunk
// These functions return 0 if they have sent or recv'd the whole message,
//  and 1 if they sucessfully sent or recv'd one chunk, but there's still some data left to send.
// also there should be methods IsAllDataRead() and IsAllDataSent()
//    (internally these methods can just check if the std::bufferRead/Write strings has been emptied, of I can create bool vars for it)
// Also there should be methods for retrieving the read data (exception of some indication of error if IsAllDataRead() is false)
// And the same for sending the data (AddDataTo"to_be_sent"Buffer() or seomthing like that)

