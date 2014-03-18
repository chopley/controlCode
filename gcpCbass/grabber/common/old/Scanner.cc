#include "gcp/control/code/unix/libunix_src/common/optcam.h"
#include "gcp/control/code/unix/libunix_src/common/scanner.h"

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/SpecificName.h"

#include "gcp/grabber/common/Channel.h"
#include "gcp/grabber/common/FrameGrabber.h"
#include "gcp/grabber/common/Flatfield.h"
#include "gcp/grabber/common/Scanner.h"
#include "gcp/grabber/common/ScannerMsg.h"
#include "gcp/grabber/common/Master.h"

using namespace gcp::grabber;
using namespace gcp::util;
using namespace std;

/**.......................................................................
 * Constructor.
 */
Scanner::Scanner(Master* parent, bool simulate) :
  parent_(parent), simulate_(simulate)
{
  fg_ = 0;

  // Initialize the array of images

  images_.resize(Channel::nChan_);
  for(unsigned iChan=0; iChan < Channel::nChan_; iChan++) {
    images_[iChan].channel_ = Channel::intToChannel(iChan);
  }

  // Now initialize the frame grabber object

  if(!simulate) {

    // Allocate the frame grabber object
    
    fg_ = new FrameGrabber();
    
    // Set the image size
    
    fg_->setImageSize(GRABBER_XNPIX, GRABBER_YNPIX);

  }

  //-----------------------------------------------------------------------
  // Network communications resources.
  //-----------------------------------------------------------------------

  // Create a TCP/IP network input stream. The longest message will be
  // the array of register blocks passed as a 4-byte address plus
  // 2-byte block dimension per register.
  
  netStr_ = 0;
  netStr_ = new NetStr(-1, SCAN_MAX_CMD_SIZE, OPTCAM_BUFF_SIZE);

  // Install handlers

  netStr_->getReadStr()->installReadHandler(netMsgReadHandler, this);
  netStr_->getSendStr()->installSendHandler(imageSentHandler, this);

  netStr_->getReadStr()->installErrorHandler(netErrorHandler, this);
  netStr_->getSendStr()->installErrorHandler(netErrorHandler, this);

  // Attempt to connect to the frame grabber port

  connectScanner(true);
}

/**.......................................................................
 * Destructor.
 */
Scanner::~Scanner() 
{
  disconnect();

  if(netStr_ != 0) {
    delete netStr_;
    netStr_ = 0;
  }

  if(fg_ != 0) {
    delete fg_;
    fg_ = 0;
  }
}

/**.......................................................................
 * A handler to be called when a message has been completely read
 */
NET_READ_HANDLER(Scanner::netMsgReadHandler)
{
  Scanner* scanner =  (Scanner*)arg;

  // Mark the transaction as finished

 scanner->parseGreeting();
}

/**.......................................................................
 * A handler to be called when a message has been completely send
 */
NET_SEND_HANDLER(Scanner::imageSentHandler)
{
  Scanner* scanner =  (Scanner*)arg;

  // Remove the descriptor from the set to be watched for writeability

  scanner->fdSet_.clearFromWriteFdSet(scanner->netStr_->getSendStr()->getFd());


  // Only start listening to the message queue again if we are done
  // sending images

  if(scanner->remainMask_ == Channel::NONE) {
    scanner->fdSet_.registerReadFd(scanner->msgq_.fd());
  } else {
    scanner->sendNextImage();
  }

}

/**.......................................................................
 * Parse a greeting message.
 */
void Scanner::parseGreeting()
{
  int opcode;                      // Message-type opcode 
  unsigned int arraymap_revision; // Register map structure revision 
  unsigned int arraymap_narchive; // The number of archive registers 

  NetReadStr* nrs = netStr_->getReadStr();

  nrs->startGet(&opcode);

  if(opcode != OPTCAM_GREETING) {
    ThrowError("Corrupt greeting message.");
  };

  nrs->endGet();

  // Remove the fd from the set to be watched for readbility

  // Now that we have successfully parsed the greeting message, any
  // further input from the control program is an error

  // fdSet_.clearFromReadFdSet(netStr_->getReadStr()->getFd());

  netStr_->getReadStr()->installReadHandler(netErrorHandler, this);
}

/**.......................................................................
 * A handler to be called when an error occurs in communication
 */
NET_ERROR_HANDLER(Scanner::netErrorHandler)
{
  Scanner* scanner =  (Scanner*)arg;
  scanner->disconnectScanner();
}

/**.......................................................................
 * Attempt to connect to the host
 */
void Scanner::connectScanner(bool reEnable)
{
  // We should only get a message to connect when we are not
  // connected, but occasionally the timing may work out so that we
  // get another poke to connect before the message has gotten through
  // to the signal task to disable the connect timer.  Thus we will
  // ignore connection requests if we are already connected

  if(client_.isConnected())
    return;

  // If we successfully connected, let the
  // parent know.

  if(connect())
    sendScannerConnectedMsg(true);

  else if(reEnable)
    sendScannerConnectedMsg(false);
}

/**.......................................................................
 * Connect to the archiver port of the control program.
 *
 * Return:
 *  return     bool    true  - OK.
 *                     false - Error.
 */
bool Scanner::connect()
{
  // Terminate any existing connection.
  
  disconnect();
  
  if(client_.connectToServer(parent_->imHost(), CP_RTO_PORT, true) < 0) {

    ReportMessage("Couldn't connect to " 
		  << gcp::util::SpecificName::controlName()
		  << " at " << parent_->imHost() << ":" 
		  << CP_RTO_PORT
		  << " (to return images)");

    return false;
  }

  // Once we've successfully achieved a connection, reconfigure the
  // socket for non-blocking I/O

  client_.setBlocking(false);

  // Attach the network I/O streams to the new client socket.
  
  netStr_->attach(client_.getFd());
  
  // Register the socket to be watched for input.  We should get a
  // greeting message after the connection is made.

  netStr_->getReadStr()->installReadHandler(netMsgReadHandler, this);

  fdSet_.registerReadFd(client_.getFd());
}

/**.......................................................................
 * Disconnect from the host.
 */
void Scanner::disconnectScanner()
{
  // Disconnect and let the parent know it should re-enable the
  // connect timer.

  disconnect();

  sendScannerConnectedMsg(false);
}

/**.......................................................................
 * Disconnect the connection to the control-program archiver.
 *
 * Input:
 *  sup    Scanner *  The resource object of the program.
 */
void Scanner::disconnect()
{
  // Clear the client fd from the set to be watched in select()

  if(client_.isConnected())
    fdSet_.clear(client_.getFd());

  // Disconnect from the server socket.

  client_.disconnect();

  // Detach network handlers

  netStr_->attach(-1);

  // Make sure we are listening to our message queue.

  fdSet_.registerReadFd(msgq_.fd());
}

/**.......................................................................
 * Service our message queue.
 */
void Scanner::serviceMsgQ()
{
  bool stop   = false;
  int  nready = 0;
  int  msgqFd = msgq_.fd();

  // Register the msgq to be watched for readability  

  fdSet_.registerReadFd(msgqFd);

  // Initially our select loop will only check the msgq file
  // descriptor for readability, but attempt to connect to the control
  // port every two seconds.  Once a connection is achieved, the
  // select loop will block until either a message is received on the
  // message queue, or on the control port.
  
  while(!stop) {
    
    nready=select(fdSet_.size(), fdSet_.readFdSet(), fdSet_.writeFdSet(), 
		  NULL, timeOut_.tVal());

    if(nready > 0) {

      // A message on our message queue?
    
      if(fdSet_.isSetInRead(msgqFd))
	processTaskMsg(&stop);
      
      // A greeting message from the archiver?
      
      if(fdSet_.isSetInRead(netStr_->getReadStr()->getFd())) {
	netStr_->read();
      }
      
      // Send a pending image to the control program
      
      if(fdSet_.isSetInWrite(netStr_->getSendStr()->getFd()))
	netStr_->send();

      // Did we time out?

    } else {

      // Only send images if we are not currently sending any.  Else
      // ignore this timeout

      if(remainMask_ == Channel::NONE)
	sendImages(intervalMask_, false);

      timeOut_.reset();
    }
  }
}

/**.......................................................................
 * Pack an image into our network buffer.
 */
void Scanner::packImage(Image& image) 
{
  TimeVal timeVal;
  timeVal.setToCurrentTime();

  image.finalize();

  unsigned int utc[2];
  unsigned int actual[3];
  unsigned int expected[3];

  utc[0] = timeVal.getMjdDays();
  utc[1] = timeVal.getMjdMilliSeconds();

  NetSendStr* nss = netStr_->getSendStr();

  unsigned short channel = Channel::channelToInt(image.channel_);

  nss->startPut(0);
  nss->putInt(2, utc);
  nss->putShort(1, &channel);
  nss->putInt(3, actual);
  nss->putInt(3, expected);
  nss->putShort(GRABBER_IM_SIZE, &image.image_[0]);
  nss->endPut();
}

/**.......................................................................
 * Get an image from the frame grabber
 */
void Scanner::addGrabberImage(Image& image) 
{
  // A utility array in which we will store the single-byte image
  // returned by the frame grabber

  vector<char> grabberImage;

  // Get the next image from the frame grabber, selecting the
  // appropriate channel first for this image

  fg_->setChannel(Channel::channelToInt(image.channel_));

  fg_->getImage(grabberImage);

  // Check that the frame grabber returned an image of the right size

  if(grabberImage.size() != GRABBER_IM_SIZE) 
    ThrowError("Bad image size returned from the frame grabber");

  // Now copy the image into the image buffer

  char*  cptr = &grabberImage[0];
  float* sptr = &image.intImage_[0];

  // Store the running mean if we are integrating images.  Don't worry
  // about pixel masks here -- we will deal with this in the
  // finalizeImage() step

  for(unsigned ipix = 0; ipix < grabberImage.size(); ipix++) {
    *sptr += ((unsigned char) *cptr++ - *sptr)/(count_+1);
    sptr++;
  }
};

/**.......................................................................
 * Process a message specific to the Task
 */
void Scanner::processMsg(ScannerMsg* msg)
{
  // Now parse the message

  switch (msg->type) {
  case ScannerMsg::CONNECT:

    // Attempt to connect to the scanner port.  On success, disable
    // the scanner_connect timer.

    connectScanner(false);
    break;
  case ScannerMsg::CONFIGURE:

    //------------------------------------------------------------
    // Configure the frame timeout
    //------------------------------------------------------------

    if(msg->body.configure.mask & gcp::control::FG_INTERVAL) {

      if(msg->body.configure.seconds == 0) {
	timeOut_.activate(false);
      } else {
	timeOut_.setIntervalInSeconds(msg->body.configure.seconds);
	timeOut_.activate(true);
	intervalMask_ = msg->body.configure.channelMask;
      }

    }

    //------------------------------------------------------------
    // Acquire an image, either as a flatfield, or a simple image
    //------------------------------------------------------------

    if(msg->body.configure.mask & gcp::control::FG_TAKE_FLATFIELD) {
      COUT("Saving image as flatfield: " << msg->body.configure.channelMask);
      sendImages(msg->body.configure.channelMask, true);

    }
    
    if(msg->body.configure.mask & gcp::control::FG_TAKE_IMAGE) {
      COUT("Taking image: " << msg->body.configure.channelMask);
      sendImages(msg->body.configure.channelMask, false);
    }

    //------------------------------------------------------------
    // For the rest of these, they can be specified for multiple
    // channels
    //------------------------------------------------------------

    for(unsigned iChan=0; iChan < Channel::nChan_; iChan++) {

      if(images_[iChan].channel_ & msg->body.configure.channelMask) {

	gcp::grabber::Image* ih = &images_[iChan];

	if(msg->body.configure.mask & gcp::control::FG_COMBINE) 
	  ih->nCombine_ = msg->body.configure.nCombine;

	if(msg->body.configure.mask & gcp::control::FG_FLATFIELD)
	  ih->flatfield_ = msg->body.configure.flatfield;

      }

    }

    break;

  default:
    throw Error("Control::processMsg: Unrecognized message type.\n");
    break;
  }
}

/**.......................................................................
 * Pack an image for transmission to the frame grabber port.
 */
void Scanner::sendImages(unsigned chanMask, bool storeAsFlatfield)
{
  storeAsFlatfield_ = storeAsFlatfield;
  remainMask_ = chanMask;
  sendNextImage();
}


/**.......................................................................
 * Send the next image in the queue
 */
void Scanner::sendNextImage()
{
  for(unsigned iChan=0; iChan < Channel::nChan_; iChan++) {

    if(images_[iChan].channel_ & remainMask_) {
      sendImage(images_[iChan], storeAsFlatfield_);
      remainMask_ &= ~images_[iChan].channel_;
      return;
    }
  }

  return;
}


/**.......................................................................
 * Pack an image for transmission to the frame grabber port.
 */
void Scanner::sendImage(Image& image, bool storeAsFlatfield)
{
  // If the client isn't connected, do nothing.
  
  if(!client_.isConnected())
    return;

  // We will queue further messages to this task until the image is
  // sent

  fdSet_.clearFromReadFdSet(msgq_.fd());

  // Acquire the images

  for(count_=0; count_ < image.nCombine_; count_++) {

    // Acquire the next image
    
    if(simulate_) {
      image.addFakeImage();
    } else {
      addGrabberImage(image);
    }
  }

  // If we were requested to store this image as a flatfield, do so
  // now Do it before we call packImage, which will apply flatfielding
  // to the image, which we don't want for a flatfield image

  if(storeAsFlatfield)
    image.storeCurrentImageAsFlatfield();

  // Pack the image into our network buffer

  packImage(image);  

  // Add the descriptor to be watched for writability
  
  fdSet_.registerWriteFd(netStr_->getSendStr()->getFd());
}

/**.......................................................................
 * A method to send a connection status message to the
 * Master object.
 */
void Scanner::sendScannerConnectedMsg(bool connected)
{
  MasterMsg msg;
  
  msg.packScannerConnectedMsg(connected);
  
  parent_->forwardMasterMsg(&msg);
}


