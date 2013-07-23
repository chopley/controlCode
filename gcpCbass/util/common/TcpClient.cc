#define __FILEPATH__ "util/common/TcpClient.cc"

#include <sys/socket.h>
#include <unistd.h>

#include "gcp/control/code/unix/libunix_src/common/tcpip.h"

#include "gcp/util/common/TcpClient.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
TcpClient::TcpClient()
{
  fd_ = -1;
  connected_   = false;
  hostInit_    = false;
  portInit_    = false;
};

/**.......................................................................
 * Constructor.
 */
TcpClient::TcpClient(std::string host, unsigned port) 
{
  fd_ = -1;
  connected_   = false;

  setHost(host);
  setPort(port);
};

/**.......................................................................
 * Destructor.
 */
TcpClient::~TcpClient() 
{
  disconnect();
};

/**.......................................................................
 * Constructor.
 */
int TcpClient::connectToServer(std::string host, unsigned port, bool doWait) 
{
  setHost(host);
  setPort(port);

  return privateConnectToServer(doWait);
};

/**.......................................................................
 * Connect to the server
 */
int TcpClient::connectToServer(bool doWait)
{
  return privateConnectToServer(doWait);
};

/**.......................................................................
 * Connect to the server
 */
int TcpClient::privateConnectToServer(bool doWait)
{
  if(!(hostInit_ && portInit_))
    ThrowError("Host and port number have not been set");

  // Terminate any existing connection.
  
  disconnect();

  // Now attempt to connect.

  try {
    fd_ = tcp_connect((char *)host_.c_str(), port_, doWait);
  } catch(Exception& err) {
  }
  
  if(fd_ < 0) {
    return -1;
  }
  
  // If tcp_connect() successfully returned an fd mark us as connected.
  
  connected_ = true;

  return fd_;
};

/**.......................................................................
 * Disconnect the connection to the control-program control port.
 */
void TcpClient::disconnect()
{
  if(fd_ >= 0) {
    shutdown(fd_, 2);
    close(fd_);
    fd_ = -1;
  };

  connected_ = false;
}

/**.......................................................................
 * Set the host
 */
void TcpClient::setHost(std::string host)
{
  host_     = host;
  hostInit_ = true;
}

/**.......................................................................
 * Set the port
 */
void TcpClient::setPort(unsigned port)
{
  port_     = port;
  portInit_ = true;
}

/**.......................................................................
 * Return true if we are connected
 */
bool TcpClient::isConnected()
{
  return connected_;
}

/**.......................................................................
 * Configure the socket managed by this class for
 * blocking/non-blocking I/O
 */
void TcpClient::setBlocking(bool doWait)
{
  if(tcp_set_blocking(fd_, doWait)) 
    ThrowError("Unable to reconfigure socket");
}
