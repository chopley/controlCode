#define __FILEPATH__ "util/common/TcpListener.cc"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h> // close()

#include "gcp/control/code/unix/libunix_src/common/tcpip.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TcpListener.h"

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
TcpListener::TcpListener(unsigned port, unsigned queueSize) 
{
  LogStream errStr;

  fd_ = -1;

    // Attempt to allocate a TCP/IP port on which to listen for
  // connection requests from the antenna computers.

  fd_ = tcp_server_sock(port, queueSize);

  if(fd_ < 0) {
    ThrowSysError("Unable to allocate the server socket  port = " << port);
  }
};

/**.......................................................................
 * Destructor.
 */
TcpListener::~TcpListener() 
{
  if(fd_ >= 0)
    close(fd_);
};

/**.......................................................................
 * This function is called to accept or reject a connection request
 * from the real-time controller.
 */
int TcpListener::acceptConnection(bool blocking)
{
  struct sockaddr_in client_addr;/* The address of the client */
  int addr_len;                  /* The size of client_addr */
  int fd;                        /* The socket assigned to the client */
  LogStream errStr;

  // Allow the caller to connect.

  addr_len = (int)sizeof(client_addr);
  fd = accept(fd_, 
	      (struct sockaddr *) &client_addr, 
	      (socklen_t* )&addr_len);

  if(fd < 0) {
    errStr.appendSysError(true, "accept()");
    throw Error(errStr);
  };

  // Report the connection for security purposes.

  cout << "Accepted connection from: " << 
    inet_ntoa(client_addr.sin_addr) << endl;

  // Set up for non-blocking I/O

  tcp_set_blocking(fd, blocking);

  // Return the file descriptor

  return fd;
}
