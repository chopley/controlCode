#define __FILEPATH__ "mediator/specific/OptCam.cc"

#include <cmath>
#include <sys/socket.h> // Needed for shutdown()

#include "gcp/mediator/specific/Master.h"
#include "gcp/mediator/specific/OptCam.h"

#include "gcp/control/code/unix/libunix_src/common/tcpip.h"

using namespace std;
using namespace gcp::mediator;
using namespace gcp::util;

//=======================================================================
// OptCam class
//=======================================================================

/**.......................................................................
 * Constructor for the OptCam encapsulation
 */
OptCam::OptCam(Master* master)
{
  // Sanity check
  
  if(master == 0)
    throw Error("OptCam::OptCam: "
		"Received NULL master argument.\n");
  
  // Initialize the optical camera encapsulation
  
  fd_               = -1;
  connected_        = false;
  ready_            = false;
  nrs_              = NULL;
  nss_              = NULL;
  image_net_buffer_ = NULL;
  image_            = NULL;
  master_           = master;
  
  fd_set_size_ = 0;
  FD_ZERO(&(read_fds_));
  FD_ZERO(&(write_fds_));
  
  // Create an unassigned TCP/IP network input stream for the control
  
  nrs_ = new NetReadStr(-1, OPTCAM_MAX_CMD_SIZE);
  
  if(nrs_ == 0)
    throw Error("OptCam::OptCam: Insufficient memory.\n");
  
  // Create a TCP/IP network stream. Request a buffer size of zero so
  // that the image buffer can be dynamically passed in as the network
  // buffer.
  
  nss_ = new NetSendStr(-1, 0);
  
  if(nss_ == 0)
    throw Error("OptCam::OptCam: Insufficient memory.\n");
  
  // Allocate image buffers
  
  if((image_net_buffer_ = (unsigned char *)malloc(OPTCAM_BUFF_SIZE))==0)
    throw Error("OptCam::OptCam: Insufficient memory.\n");
  
  if((image_ = (unsigned short *)malloc(sizeof(unsigned short)*
					GRABBER_IM_SIZE))==0)
    throw Error("OptCam::OptCam: Insufficient memory.\n");
}

/**.......................................................................
 * OptCam destructor function
 */
OptCam::~OptCam()
{
  
  disconnect();
  
  if(nrs_ != 0) {
    delete nrs_;
    nrs_ = 0;
  }
  
  if(nss_ != 0) {
    delete nss_;
    nss_ = 0;
  }
  
  if(image_net_buffer_ != 0)
    free(image_net_buffer_);
  
  if(image_ != 0)
    free(image_);
}

/**.......................................................................
 * Connect to the optical camera port of the control program.
 */
bool OptCam::connect()
{
  int on = 1;      // The argument of the FIONBIO ioctl() 
  
  // Terminate any existing connection.
  
  disconnect();
  fd_ = tcp_connect((char *)master_->ctlHost().c_str(), CP_RTO_PORT, 1);
  
  // If tcp_connect() successfully returned an fd, add it to the list
  // to be watched for input.
  
  if(fd_ >= 0) {
    
    connected_ = true;
    
    // Arrange for I/O on the control-program socket to be non-blocking.
    
    if(ioctl(fd_, FIONBIO, reinterpret_cast<long int>(&on))) {
      
      perror("ioctl:");
      disconnect();
      
      ErrorDef(err, "OptCam::connect: "
	       "Unable to select non-blocking I/O.\n");
      return true;
    }
    
    // Report the connection.
    
    cout << "OptCam Connection established to the control program.\n" 
	 << endl;
    
    // Attach the network I/O streams to the new client socket.
    
    nrs_->attach(fd_);
    nss_->attach(fd_);
    
    registerReadFd(fd_);
    return false;
  } else {
    ErrorDef(err, "OptCam::connect: "
	     "Error in tcp_connect().\n");
    return true;
  }
}

/**.......................................................................
 * Disconnect the connection to the control-program optical camera port.
 *
 * Input:
 *  sup     *  The resource object of the program.
 */
void OptCam::disconnect()
{
  if(fd_ >= 0) {
    shutdown(fd_, 2);
    close(fd_);
    nrs_->attach(-1);
    nss_->attach(-1);
    fd_ = -1;
  };
}

/**.......................................................................
 * Private method to register a file descriptor to be watched for
 * input
 */
void OptCam::registerReadFd(int fd)
{
  // And register this descriptor to be watched for input
  
  FD_SET(fd, &(read_fds_));
  
  if(fd + 1 > fd_set_size_)
    fd_set_size_ = fd + 1;
}

/**.......................................................................
 * Private method to register a file descriptor to be watched for
 * input
 */
void OptCam::registerWriteFd(int fd)
{
  // And register this descriptor to be watched for input
  
  FD_SET(fd, &(write_fds_));
  
  if(fd + 1 > fd_set_size_)
    fd_set_size_ = fd + 1;
}

/*.......................................................................
 * Read data from the current TCP/IP socket. Note that if the socket
 * blocks then the function returns immediately and should be called
 * again later when more data becomes available.
 *
 * @throws Exception
 */
void OptCam::readCommand()
{
  int opcode;          // The message-type opcode 
  
  // Read as much data as possible from the socket.
  
  switch(nrs_->read()) {
  case NetReadStr::NET_READ_SIZE:  // Read blocked before completion 
  case NetReadStr::NET_READ_DATA:
    break;
  case NetReadStr::NET_READ_DONE:  // Read completed 
    
    // After the client has sent an OPTCAM_GREETING message then
    // optcam.ready is set to true and no more messages are allowed.
    
    if(ready_) 
      throw Error("OptCam::readCommand: Unexpected message.\n");
    
    // Read the message type.
    
    nrs_->startGet(&opcode);
    
    // Determine the message type. Note that the cast to a
    // ClientToOptCam enumeration makes it possible for the compiler
    // to tell us if there any enumerators missing from the switch.
    
    switch((CpToOptCam) opcode) {
    case OPTCAM_GREETING:
      
      // Read the end of the message.
      
      nrs_->endGet();
      
      // Initiate sending images.
      
      ready_ = true;
      
      // No further input is expected from the control program.
      
      clearFromReadFd(fd_);
      break;
    };
    break;
  case NetReadStr::NET_READ_ERROR:// I/O error or remote close of the connection
  case NetReadStr::NET_READ_CLOSED:
    throw Error("OptCam::readCommand: "
		"Connection to the optical camera closed.\n");
    break;
  };
}

/**.......................................................................
 * Randomly generate a field of stars
 */
void OptCam::fillImage()
{
  // Randomly generate some sources
  
  int i,irow,icol,ind;
  float xval,yval,val;
  
  // Randomly generate an integer between 1 and 10
  
  int nran = 1+(int)(10.0*rand()/(RAND_MAX+1.0));
  int iran[10],jran[10];
  float fluxran[10];
  
  // randomly generate positions for the sources
  
  for(i=0;i < nran;i++) {
    iran[i] = (int)((GRABBER_XNPIX-1.0)*rand()/(RAND_MAX+1.0));
    jran[i] = (int)((GRABBER_YNPIX-1.0)*rand()/(RAND_MAX+1.0));
    
    // And randomly generate a flux for this source, between 10 and
    // 100
    
    fluxran[i] = 10.0+(90.0*rand()/RAND_MAX);
  }
  
  for(irow=0;irow < GRABBER_YNPIX;irow++)
    for(icol=0;icol < GRABBER_XNPIX;icol++) {
      ind = irow*GRABBER_XNPIX + icol;
      val = 0.0;
      for(i=0;i < nran;i++) {
	xval = iran[i] - irow;
	yval = jran[i] - icol;
	val += fluxran[i]*exp(-(xval*xval + yval*yval)/(2*10*10));
      }
      image_[ind] = (unsigned short)val;
    }
}

/**.......................................................................
 * Send an image from the "frame grabber"
 */
void OptCam::sendImage()
{
  if(nss_->state() == NetSendStr::NET_SEND_DONE) {    
    fillImage();
    
    nss_->setBuffer((unsigned int*)image_net_buffer_, OPTCAM_BUFF_SIZE);
    nss_->startPut(0);
    nss_->putInt(2, utc_);
    nss_->putInt(3, (unsigned int *)actual_);
    nss_->putInt(3, (unsigned int *)expected_);
    nss_->putShort(GRABBER_IM_SIZE, image_);
    nss_->endPut();
  }
  
  // Send as much of the oldest image as possible. Note that it may
  // take many calls to completely send an image.
  
  switch(nss_->send()) {
  case NetSendStr::NET_SEND_DATA:   // Image partially sent before blocking 
    break;
  case NetSendStr::NET_SEND_DONE:   // Image completely sent 
    clearFromWriteFd(fd_);
    break;
  case NetSendStr::NET_SEND_ERROR:  // I/O Error 
  case NetSendStr::NET_SEND_CLOSED:
    throw Error("OptCam::sendImage: Connection closed.\n");
    break;
  };
}

/**.......................................................................
 * Return true if the scanner connection is open
 */
bool OptCam::isConnected() 
{
  return connected_;
}

#ifdef OLD
/**.......................................................................
 * OptCam main event loop
 */
void OptCam::run()
{
  bool waserr=false;
  int nready=0;
  ostringstream os;
  
  // Block until a connection is made to the control program,
  // sleeping between attempts
  
  while(!isConnected())
    if(connect())
      sleep(2);
  
  // Now block in select until either a command is received, or it's
  // time to send a frame
  
  while(!waserr) {
    
    FD_ZERO(&(read_fds_));
    FD_SET(fd_, &(read_fds_));
    
    nready = select(fd_set_size_, &(read_fds_), &(write_fds_), 0, 0);
    
    switch (nready) {
    case 2: 
    case 1: // Deliberate fall-through here
      
      // A greeting message on the optical camera socket?
      
      if(FD_ISSET(fd_, &(read_fds_))) 
	readCommand();
      
      // Is the optical camera socket ready for more data to be
      // sent?
      
      if(FD_ISSET(fd_, &(write_fds_))) 
	sendImage();
      
      break;
    default:
      os << "OptCam::run: Error in select().\n" << ends;
      waserr = true;
      break;
    }
  }
  
  // Disconnect if an error occurred
  
  disconnect();
  
  // Throw an error if one occurred
  
  if(waserr)
    throw Error(os.str());
}
#else
/**.......................................................................
 * OptCam main event loop
 */
void OptCam::run()
{
  serviceMsgQ();
}
#endif

/**.......................................................................
 * Remove a file descriptor from the mask of descriptors to be watched
 * for readability
 */
void OptCam::clearFromReadFd(int fd) {
  FD_CLR(fd, &(read_fds_)); 
}

/**.......................................................................
 * Remove a file descriptor from the mask of descriptors to be watched
 * for readability
 */
void OptCam::clearFromWriteFd(int fd)
{
  FD_CLR(fd, &(write_fds_));
}

void OptCam::processMsg(OptCamMsg* taskMsg) {}
