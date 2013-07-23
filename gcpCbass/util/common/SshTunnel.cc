#include "gcp/util/common/CoProc.h"
#include "gcp/util/common/FdSet.h"
#include "gcp/util/common/Port.h"
#include "gcp/util/common/SshTunnel.h"
#include "gcp/util/common/TimeOut.h"

#include<iostream>

#include<errno.h>
#include<string.h>

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructor to ssh tunnel to a port (port) on a machine
 * (host) behind a gateway (gateway) machine.  If there is no
 * gateway between us and the host, use the next constructor, or
 * specify the same argument for both
 */
SshTunnel::SshTunnel(std::string gateway, std::string host, 
		     unsigned short port, 
		     unsigned timeOutInSeconds)
{
  initialize(gateway, host, port, port, timeOutInSeconds);
}

/**.......................................................................
 * Constructor to ssh tunnel to a port (port) on a machine
 * (host) behind a gateway (gateway) machine.  If there is no
 * gateway between us and the host, use the next constructor, or
 * specify the same argument for both.
 */
SshTunnel::SshTunnel(std::string gateway, std::string host, 
		     unsigned short port, unsigned short hostPort, 
		     unsigned timeOutInSeconds)
{
  initialize(gateway, host, port, hostPort, timeOutInSeconds);
}

/**.......................................................................
 * Constructor to tunnel to a port on the specified machine
 */
SshTunnel::SshTunnel(std::string host, unsigned short port, 
		     unsigned timeOutInSeconds) 
{
  initialize(host, host, port, port, timeOutInSeconds);
}

/**.......................................................................
 * Constructor to tunnel to a port on the specified machine
 */
SshTunnel::SshTunnel(std::string host, unsigned short port, unsigned short hostPort, 
		     unsigned timeOutInSeconds) 
{
  initialize(host, host, port, hostPort, timeOutInSeconds);
}

/**.......................................................................
 * Most of the work of this class is performed in the initialize
 * method.  This just sets up the object and fires off the tunnel,
 * blocking if a non-zero timeout was specified
 */
void SshTunnel::initialize(std::string gateway, std::string host, unsigned short port, unsigned short hostPort, 
			   unsigned timeOutInSeconds)
{
  success_  = false;
  gateway_  = gateway;
  host_     = host;
  port_     = port;
  hostPort_ = hostPort;
  proc_     = 0;
  error_ << "Unknown";

  spawn();

  if(timeOutInSeconds > 0) {
    blockUntilSuccessOrTimeOut(timeOutInSeconds);
  }

}

/**.......................................................................
 * Launch the process that creates the ssh tunnel
 */
void SshTunnel::spawn()
{
  std::ostringstream os;

  os << "ssh -vL " << port_ << ":" << host_ << ":" << hostPort_ 
     << " " << gateway_;

  COUT("Executing coproc with: " << os.str());

  proc_ = new CoProc(os.str());
}

/**.......................................................................
 * For convenience of code that instantiates this object, this method
 * allows the calling thread to block until the tunnel either
 * succeeds, or until the specified time has elapsed.
 *
 * The caller can query the success status by using the succeeded()
 * method of this class.
 */
void SshTunnel::blockUntilSuccessOrTimeOut(unsigned timeOutInSeconds)
{
  TimeOut timeOut;
  timeOut.setIntervalInSeconds(timeOutInSeconds);
  timeOut.activate(true);
  
  FdSet fdSet;
  int stdOutFd = proc_->stdOut()->readFd();
  int stdErrFd = proc_->stdErr()->readFd();

  fdSet.registerReadFd(stdOutFd);
  fdSet.registerReadFd(stdErrFd);

  std::ostringstream osStdout, osStderr;
  bool stdOutIsConnected = true;
  bool stdErrIsConnected = true;

  bool stop=false;
  while(!stop) {

    int nready = select(fdSet.size(), fdSet.readFdSet(), 0, 0, timeOut.tVal());

    // Error in select
    
    if(nready < 0) {
      error_.str("");
      error_ << "In select(): " << strerror(errno);
      success_ = false;
      stop     = true;

      // Timed out
      
    } else if(nready == 0) {
      error_.str("");
      error_ << "ssh timed out.  The network may be slow (or unreachable), " 
	     << "the remote ssh server may be prompting you for a password, "
	     << "or you may have specified an unrecognized gateway ("
	     << gateway_ << ") " << "or host name (" << host_ << "). "
	     << "Try the command: "
	     << std::endl << std::endl << "     " << *this << std::endl
	     << "on the command line and see for yourself.";

      success_ = false;
      stop     = true;

      // Response from ssh server, or disconnect
      
    } else if(fdSet.isSetInRead(stdOutFd)) {
      parseStdOutMsg(stdOutFd, osStdout, stdOutIsConnected, stop);
    } else if(fdSet.isSetInRead(stdErrFd)) {
      parseStdErrMsg(stdErrFd, osStderr, stdErrIsConnected, stop);
    }

    // If after parsing responses from stdout or stderr, both
    // descriptors have hung up, stop trying to read

    if(!stdOutIsConnected)
      fdSet.clearFromReadFdSet(stdOutFd);

    if(!stdErrIsConnected)
      fdSet.clearFromReadFdSet(stdErrFd);

    if(!stdOutIsConnected  && !stdErrIsConnected)
      stop = true;
  }

  //COUT("From stdout: " << osStdout.str());
  //  COUT("From stderr: " << osStderr.str());
}    

/**.......................................................................
 * Parse a message received on stdout
 */
void SshTunnel::parseStdOutMsg(int fd, std::ostringstream& os, bool& connected, bool& stop)
{
  Port port(fd);

  if(port.getNbyte() > 0) {
    port.concatenateString(os);
  } else {
    connected = false;
  }
}

/**.......................................................................
 * Parse a message received on stderr
 */
void SshTunnel::parseStdErrMsg(int fd, std::ostringstream& os, bool& connected, bool& stop)
{
  Port port(fd);

  if(port.getNbyte() > 0) {
    port.concatenateString(os);
  } else {
    connected = false;
  }

  String str(os.str());

  if(str.contains("Could not resolve hostname")) {
    stop     = true;
    success_ = false;
    error_.str("");
    error_ << "Hostname: " << gateway_ << " couldn't be resolved";
  } else if(str.contains("Name or service not known")) {
    stop     = true;
    success_ = false;
    error_.str("");
    error_ << gateway_ << ": Name or service not known";
  } else if(str.contains("Address already in use")) {
    stop     = true;
    success_ = false;
    error_.str("");
    error_ << "Port: " << port_ << " is already in use";
  } else if(str.contains("Next authentication method: password")) {
    stop     = true;
    success_ = false;
    error_.str("");
    error_ << "The ssh server on " << gateway_ << " is prompting you for a password";
  } else if(str.contains("Entering interactive session")) {
    stop     = true;
    success_ = true;
  }

}

/**.......................................................................
 * Output Operator.
 */
std::ostream& gcp::util::operator<<(std::ostream& os, SshTunnel& obj)
{
  os << "ssh -vL " << obj.port_ << ":" << obj.host_ << ":" << obj.hostPort_ 
     << " " << obj.gateway_ << std::endl;
 
  return os;
};

/**.......................................................................
 * Destructor.
 */
SshTunnel::~SshTunnel() 
{
  if(proc_) {
    delete proc_;
    proc_ = 0;
  }
}

/**.......................................................................
 * Return true if the tunnel succeeded
 */
bool SshTunnel::succeeded()
{
  return success_;
}

std::string SshTunnel::error()
{
  return error_.str();
}

