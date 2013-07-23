#include <sys/socket.h>
#include <errno.h>
#include <string.h>

#include "gcp/control/code/unix/libunix_src/common/tcpip.h"

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/antenna/control/specific/AntennaDrive.h"
#include "gcp/antenna/control/specific/AntennaException.h"
#include "gcp/antenna/control/specific/PmacComms.h"
#include "gcp/antenna/control/specific/SpecificShare.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor with shared resources.
 */
PmacComms::PmacComms(SpecificShare* share)
{
  // Sanity check the share argument.

  if(share == 0)
    throw Error("PmacComms::PmacComms: share argument is NULL.\n");

  share_       = share;
  fd_          = -1;
  connected_   = false;
}

/**.......................................................................
 * Constructor with no shared resources.
 */
PmacComms::PmacComms()
{
  share_       = 0;
  fd_          = -1;
  connected_   = false;
}

/**.......................................................................
 * Destructor
 */
PmacComms::~PmacComms()
{
  // Disconnect from the PMAC port

  disconnect();
}

/**.......................................................................
 * Send a command to the pmac.
 */
void PmacComms::sendCommand(PmacCommand& command)
{
  // Don't try to send commands if we are not connected.

  if(!connected_)
    return;

  // Check that this is a valid command.

  if(command.request() == PmacCommand::INVALID)
    throw Error("PmacComms::readCommand:"
		"Sending the PMAC an invalid command.");

  int status = send(fd_, command.cmd(), command.size(), 0);

  // send() should return the number of bytes requested to be sent.
  
  if(status != command.size()) {
    ostringstream os;
    os << "PmacComms::sendCommand: " << strerror(errno) 
       << " in write()." << endl << ends;
    throw Error(os.str());
  }
}

/**.......................................................................
 * Private version to read a response into our internal message container.
 */
void PmacComms::readResponse(PmacCommand& command)
{
  // Check if we previously sent a valid command.

  if(command.request() == PmacCommand::INVALID)
    throw Error("PmacComms::readCommand:"
		"Attempt to read a response before any command was sent.");

  // Complain if the command that was previously sent didn't expect a
  // response.
  
  if(!command.expectsResponse_)
    throw Error("PmacComms::readCommand:"
		"No reponse expected to the last sent command.\n");
    
  // Now attempt to receive the next message from the socket.

  int status = recv(fd_, command.readData(), command.responseLength(), 0);

  // If the number of bytes read was not what we expected, complain.

  if(status != command.responseLength()) {
    ostringstream os;
    os << "PmacComms::readResponse: " << strerror(errno) 
       << " in write()." << endl << ends;
    throw Error(os.str());
  }
}

/**.......................................................................
 * Disconnect from the pmac port.
 */
void PmacComms::disconnect()
{
  // Before we are connected, the fd will be initialized to -1

  if(fd_ >= 0) {

    if(shutdown(fd_, 2) < 0) {
      ostringstream os;
      os << "PmacComms::disconnect: " << strerror(errno) << " in shutdown()."
	 << endl << ends;
      ErrorDef(err, os.str());
    }

    if(close(fd_) < 0) {
      ostringstream os;
      os << "PmacComms::disconnect: " << strerror(errno) << " in close()."
	 << endl << ends;
      ErrorDef(err, os.str());
    }
  }

  // And set these to indicate we are disconnected.

  fd_        = -1;
  connected_ = false;
}

/**.......................................................................
 * Connect to the pmac port.
 */
bool PmacComms::connect()
{
  // Return immediately if we are already connected.

  if(connected_)
    return true;

  // Get a UDP connection to the host port 

  fd_ = udp_connect(PMAC_HOST_ADDR, PMAC_HOST_PORT, 1);
  
  if(fd_ < 0) {
    ErrorDef(err, "PmacComms::connect: Error in udp_connect().\n");
    return false;
  }
  
  DBPRINT(true, Debug::DEBUG31, "Connected to the PMAC");

  connected_  = true;

  return true;
}

/**.......................................................................
 * Private method to zero the set of descriptors to be watched for
 * readability.
 */
void PmacComms::zeroReadFds()
{
  // And register this descriptor to be watched for input
  
  FD_ZERO(&read_fds_);
}

/**.......................................................................
 * Private method to register a file descriptor to be watched for
 * input.
 */
void PmacComms::registerReadFd(int fd)
{
  // And register this descriptor to be watched for input
  
  FD_SET(fd, &read_fds_);
  
  if(fd + 1 > fd_set_size_)
    fd_set_size_ = fd + 1;
}

/**.......................................................................
 * Wait for a response from the pmac
 */
int PmacComms::waitForResponse()
{
  TimeVal timeout(0, PMAC_TIMEOUT_USEC, 0);
  fd_set read_fds;
  int fd_set_size;

  // Do nothing if we are not connected to the pmac.

  if(!connected_) {

    DBPRINT(true, DEBUG_PMAC, "Not connected -- returning 0");

    return 0;
  }

  // read_fds_ seems to be modified by select(), at least on
  // timeout.  This is not documented, so it appears to be a bug.
  // In any event, unless we explicitly reset it after a return, it
  // can become undefined, so we do this on each pass through the
  // while loop.
  
  FD_ZERO(&read_fds);

  // If we are connected to the pmac, register its socket 
  
  FD_SET(fd_, &read_fds);

  int nready = select(fd_+1, &read_fds, NULL, NULL, timeout.timeVal());

  if(Debug::debugging(DEBUG_PMAC)) {
    if(nready > 0) {
      DBPRINT(true, DEBUG_PMAC, "Pmac responded in: " 
	      << timeout.getElapsedTimeInMicroSeconds() 
	      << " microseconds");
    } else if(nready == 0) {
      DBPRINT(true, DEBUG_PMAC, "Pmac timed out after: " 
	      << PMAC_TIMEOUT_USEC 
	      << " microseconds");
    }
  }

  // If select generated an error, throw it

  if(nready < 0) {
    LogStream errStr;
    errStr.appendSysError(true, "select()");
    throw Error(errStr);
  }

  return nready;
}

/**.......................................................................
 * Write to a named pmac register.
 */
void PmacComms::writeReg(RegMapBlock *blk, 
			 unsigned int first, unsigned int nreg, 
			 unsigned int *value)
{
  // Do nothing if we are not connected.

  if(!connected_)
    return;

  DBPRINT(true, Debug::DEBUG31, "Got to PmacComms::writeReg()");

  // Pack the command to be sent to the pmac.

  command_.packWriteRegCmd(blk, first, nreg, value);

  getResponse(command_);
}

/**.......................................................................
 * Read from a named pmac register.
 */
void PmacComms::readReg(RegMapBlock *blk, 
			unsigned int first, unsigned int nreg, 
			unsigned int *value)
{
  // Pack the command to be sent to the pmac.

  command_.packReadRegCmd(blk, first, nreg);

  // Send the command.

  if(getResponse(command_) != 0) {

    // Now the response will be in command_'s internal buffer.  We
    // call its readRegResponse() method to unpack it into value.
    
    command_.readRegResponse(blk, first, nreg, value);

    // Then write it to shared memory.

    writeReg(blk, first, nreg, value);
  }
}

/**.......................................................................
 * Retrieve a block of memory from the pmac dpram.
 */
void* PmacComms::getMem(unsigned short offset, 
			unsigned short length)
{
  // Pack the command to be sent to the pmac.

  command_.packPmacGetMemCmd(offset, length);

  // Send the command.

  return getResponse(command_);
}

/**.......................................................................
 * Write a block of memory to the pmac dpram.
 */
void* PmacComms::setMem(unsigned short offset, 
			unsigned short length,
			unsigned char* data)
{
  // Pack the command to be sent to the pmac.

  command_.packPmacSetMemCmd(offset, length, data);

  // Send the command.

  return getResponse(command_);
}

/**.......................................................................
 * Send a command to the pmac, and return its response.
 *
 * Note -- this returns NULL if we are not connected.
 */
void* PmacComms::getResponse(PmacCommand& command)
{
  // Do nothing if we are not connected.

  if(!connected_)
    return 0;

  // Send the command.

  sendCommand(command);

  // Wait for a response.

  int nready = waitForResponse();

  if(nready != 1) {
    LogStream logStr;
    logStr.appendMessage(true, "No response from the pmac\n");
    throw Error(logStr);
  }

  // If we got a response, read it.  This will throw an error if the
  // response was not appropriate.

  readResponse(command);

  return command.readData();
}

/**.......................................................................
 * Return true if we are connected to the pmac.
 */
bool PmacComms::pmacIsConnected()
{
  return connected_;
}
