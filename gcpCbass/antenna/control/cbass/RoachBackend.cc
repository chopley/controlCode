#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>


#include "gcp/control/code/unix/libunix_src/common/tcpip.h"

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"
#include "gcp/util/common/Port.h"

#include "gcp/antenna/control/specific/AntennaException.h"
#include "gcp/antenna/control/specific/SpecificShare.h"
#include "gcp/antenna/control/specific/RoachBackend.h"

#include <fcntl.h>

//Added by CJC 18 June 2010
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <string.h>
//extern "C" size_t strlen(const char*);

////////////////////////

/*
 * Assign a default serial port device
 */
#define DEFAULT_PORT_NUMBER 12345
#define FFT_IN_CC 128
//#define DEFAULT_CONTROLLER_NAME "pumba"

using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor with share objects
 */
RoachBackend::RoachBackend(SpecificShare* share, std::string name, bool sim, char* controllerName, int roachNum):Board(share,name)
{
  //  Assign3DRingMemory();
  Assign2DRingMemory();
  sim_         =   sim;
  fd_          =    -1;
  connected_   = false;
  controllerName_ = controllerName;
  roachIndex_  = roachNum;
  missedCommCounter_=0;	
  currentIndex_ = 0;
  prevSecStart_ = 0;
  prevSecEnd_   = 0;
  thisSecStart_ = 0;
  prevTime_     = 0;

  roachUtc_       = 0;
  roachVersion_   = 0;
  roachCount_     = 0;
  roachIntLength_ = 0;
  roachBufferBacklog_ =0; 
  roachMode_      = 0;
  roachSwitchStatus_      = 0;
  roachLL_        = 0;
  roachRR_        = 0;
  roachQ_         = 0;
  roachU_         = 0;
  roachTL1_       = 0;
  roachTL2_       = 0;
  roachChan_      = 0;
  roachLLfreq_    = 0;
  roachRRfreq_    = 0; 
  roachQfreq_     = 0;  
  roachUfreq_     = 0;  
  roachTL1freq_   = 0;
  roachTL2freq_   = 0;
  roachLLtime_    = 0; 
  roachRRtime_    = 0; 
  roachQtime_     = 0;  
  roachUtime_     = 0;  
  roachTL1time_   = 0;
  roachTL2time_   = 0;
  roachNTPSeconds_   = 0;
  roachNTPuSeconds_=0;
  roachFPGAClockStamp_=0; 
  roachCoffs_=0;       
 
  roachUtc_       = findReg("utc");
  roachVersion_   = findReg("version");
  roachCount_     = findReg("intCount");
  roachIntLength_ = findReg("intLength");
  roachMode_      = findReg("mode");
  roachBufferBacklog_ = findReg("buffBacklog"); 
  roachSwitchStatus_      = findReg("switchstatus");
  roachLL_        = findReg("LL");
  roachRR_        = findReg("RR");
  roachQ_         = findReg("Q");
  roachU_         = findReg("U");
  roachTL1_       = findReg("load1");
  roachTL2_       = findReg("load2");
  roachChan_      = findReg("channel");
  roachLLfreq_    = findReg("LLfreq"); 
  roachRRfreq_    = findReg("RRfreq"); 
  roachQfreq_     = findReg("Qfreq");  
  roachUfreq_     = findReg("Ufreq");  
  roachTL1freq_   = findReg("load1freq");
  roachTL2freq_   = findReg("load2freq");
  roachLLtime_    = findReg("LLtime"); 
  roachRRtime_    = findReg("RRtime");  
  roachQtime_     = findReg("Qtime");    
  roachUtime_     = findReg("Utime");    
  roachTL1time_   = findReg("load1time");
  roachTL2time_   = findReg("load2time");
  roachNTPSeconds_   = findReg("ntpSeconds");
  roachNTPuSeconds_   = findReg("ntpUSeconds");
  roachFPGAClockStamp_   = findReg("fpgaClockStamp");
  roachCoffs_=findReg("roachCof");       



}
/**.......................................................................
 * Constructor 
 */
RoachBackend::RoachBackend(bool sim, char* controllerName)
{

  //  Assign3DRingMemory();
  Assign2DRingMemory();
  sim_         =   sim;
  fd_          =    -1;
  connected_   = false;
  controllerName_ = controllerName;

  currentIndex_ = 0;
  prevSecStart_ = 0;
  prevSecEnd_   = 0;
  thisSecStart_ = 0;
  prevTime_     = 0;

}



/**.......................................................................
 * Destructor
 */
RoachBackend::~RoachBackend()
{
  // Disconnect from the Roach board port

  disconnect();
}

/**.......................................................................
 * Connect to the roach port.
 */
bool RoachBackend::connect()
{
  //CJC 18/6/2010
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[256];
  
  ///////////
  // If we are running in simulation mode, don't try to connect to any hardware
  
  if(sim_) {
    connected_ = true;
    return true;
  }
  
  // Return immediately if we are already connected.
  
  if(connected_)
    return true;
  
  // Get a Serial Port conection to host.
  // some definitions 
  
  portno = DEFAULT_PORT_NUMBER;
  COUT("Attempting to open TCP Communication port: " << portno << " at");
  
  //CJC 18/6/2010
  
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_ < 0) {
    ReportError("Error opening TCP/IP Socket");
  }
  //  server = gethostbyname(DEFAULT_CONTROLLER_NAME);
  server = gethostbyname(controllerName_);
  if (server == NULL) {
    fprintf(stderr,"ERROR, no such host TCP/IP\n");
    exit(0);
  }

  COUT("Connection established to Roach Board Box.");
  COUT("SETTING ATTRIBUTES");
  
  //CJC 18/6/2010
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
  serv_addr.sin_port = htons(portno);
  if (::connect(fd_,(sockaddr*)&serv_addr,sizeof(serv_addr)) < 0) {   //if (connect(sockfd,&servaddr_,sizeof(servaddr_)) < 0)
    ReportError("ERROR connecting");
    connected_ = false;
    return false;
  }
  
  // printf("Please enter the message: ");
  bzero(buffer,256);
  
  DBPRINT(true, Debug::DEBUG31, "Connected to the Roach Board");
  
  connected_  = true;
  
   COUT("\nCONNECTED TO THE TCP PORT " << portno);
  /* send a first message to ensure we're talking */

  struct timespec delay;

  delay.tv_sec  =         0;
  delay.tv_nsec = 500000000;

  nanosleep(&delay, 0);

  // And add the file descriptor to the set to be watched for
  // readability and exceptions

  fdSet_.registerReadFd(fd_);
  fdSet_.registerExceptionFd(fd_);

  missedCommCounter_ = 0;

  return true;
}


/**.......................................................................
 * Disconnect from the roach port.
 */
void RoachBackend::disconnect()
{
  // Before we are connected, the fd will be initialized to -1

  // Note:  I am not reconfiguring the device back to its original state.

  connected_ = false;

  if(fd_ >= 0) {
    if(shutdown(fd_, SHUT_RDWR) <0 )
      ReportSysError("In shutdown()");

    if(close(fd_) < 0) {
      ReportSysError("In close()");
    }
  }

  // And set these to indicate we are disconnected.
  fd_        = -1;

}


/**.......................................................................
 * Return true if we are connected to the roach.
 */
bool RoachBackend::roachIsConnected()
{
  return connected_;
}

/**.......................................................................
 * Wait for a response from the roach
 */
int RoachBackend::waitForResponse()
{
  TimeVal timeout(0, ROACH_TIMEOUT_USEC, 0);
  // Do nothing if we are not connected to the roach.

  // Now wait in select until the fd becomes readable, or we time out
  int nready = select(fdSet_.size(), fdSet_.readFdSet(), NULL, fdSet_.exceptionFdSet(), timeout.timeVal());
  
  //  COUT("Dropped out of select with: nready = " << nready);

  // If select generated an error, throw it

  if(nready < 0) {
    CTOUT("In select(): File descriptor changed.  Maybe the connection was closed from other end.");
  } else if(nready==0) {
    CTOUT("Timed out in select");
  } else if(fdSet_.isSetInException(fd_)) {
    CTOUT("Exception occurred on file descriptor");
    ThrowError("Exception occurred on file descriptor");
  } else {
    //    COUT("FILE DESCRIPTOR IN SELECT CHANGED");
  }
  return nready;

}

/**............................................................
 *  Method that waits
 */
void RoachBackend::wait(int nsec) {
  
  struct timespec delay;
  delay.tv_sec = 0;
  delay.tv_nsec = nsec;

  nanosleep(&delay, NULL);
  
  return;
  
};
 


/**.......................................................................
 * Send a command to the roach.
 */
void RoachBackend::sendCommand(RoachBackendMsg& command)
{
  // Don't try to send commands if we are not connected.
  if(!connected_)
    {
    COUT("not connected");
    return;
    }

  // Check that this is a valid command.

  if(command.request_ == RoachBackendMsg::INVALID)
    ThrowError("Sending the Roach an invalid command.");

  //  COUT("MESSAGE TO SEND: " << command.messageToSend_);
  
  int status = writeString(command.messageToSend_);
  
  // writeString should return the number of bytes requested to be sent.
  //  COUT(" BYTES SENT:" << status);
  
  if(status != command.cmdSize_) {
    COUT("status, cmdSize: " << status << "," << command.cmdSize_);
    ThrowError("In writeString()");
  }
}


/**.......................................................................
 * write a message to the port
 */
int RoachBackend::writeString(std::string message)
{
  int bytesWritten = 0;
  int len = message.size();

  if(fd_>0){
    //changed CJC
    //    COUT("message to send is: " << message);
    //    COUT(" length:" << len );
    bytesWritten = write(fd_, (const char*)message.c_str(), len);//, (sockaddr *)&servaddr_, sizeof(servaddr_));
  };

  return bytesWritten;
}


/**.......................................................................
 * Private version to read a response into our internal message container.
 */
int RoachBackend::readResponse(RoachBackendMsg& command)
{
  // Check if we previously sent a valid command.
  if(command.request_ == RoachBackendMsg::INVALID)
    ThrowError("Attempt to read a response before any command was sent.");

  // Complain if the command that was previously sent didn't expect a
  // response.
  
  if(!command.expectsResponse_)
    ThrowError("No reponse expected to the last sent command.");
    
  return readTCPPort(command);

}


/***************************************************
 * Read the TCP port
 */

int RoachBackend::readTCPPort(RoachBackendMsg& command)
{
  /* return values:
     -1:  bad connection, something died.
     0:   time out in select
     1:   success
     2:   transfer took too long.
  */

  if(!roachIsConnected()){
    CTOUT("IN READTCPPORT: ROACH NOT CONNECTED");
    return 0;
  }

  if( fd_ < 0) {
    CTOUT("IN READTCPPORT: fd invalid");
    return 0;
  }

  int i,nbyte,waserr=0,nread=0;
  unsigned char line[DEFAULT_NUMBER_BYTES_PER_TRANSFER], *lptr=NULL;
  int ioctl_state=0;
  bool stopLoop = 0;
  TimeVal start, stop, diff;
  Port port;

  int bytesReceived;
  int bytesThisTransfer;
  int bytesWaiting;
  int nready;

  // Start checking how long this is taking.

  start.setToCurrentTime();

  // Set the line pointer pointing to the head of the line

  lptr = line;
  
  // See how many bytes are waiting to be read
  
  stopLoop = 0;
  bytesReceived = 0;

  do {

    try{
      nready = waitForResponse();
    } catch (...) {
      CTOUT("Exception on file desciptor.  disconnecting");
      ThrowError("Exception on file descriptor");
      stopLoop = 1;
      return -1;
    }

    if(nready == 0){
      // we had a timeout in select
      COUT("timeout on select");
      stopLoop = 1;
      return 0;
    } else if(nready < 0){
      // the file descriptor changed
      COUT("the file descriptor changed.  disconnecting");
      ThrowError("File Descriptor changed");
      stopLoop = 1;
      return -1;
    } else {
      // we try to read things in
      // find out how many bytes are available to read.
      bytesWaiting      = port.getNbyte(fd_);
      if( (bytesReceived+bytesWaiting) > command_.numBytesExpected_) {
	ReportSimpleError("Roach sending back too many bytes -- disconnecting");
	ThrowError("RoachBackend::Roach sending back too many bytes -- disconnecting");
	stopLoop=1;
	return 0; //return a 0 for this one- this will then count up for a bit before disconnecting
      }
    try{
      bytesThisTransfer = recv(fd_, lptr, bytesWaiting, 0);
    } catch (...) {
      ThrowError("RoachBackend:: Fail in call to recv-catch statement");
      CTOUT("RoachBackend:: Fail in call to recv-catch statement");
      stopLoop = 1;
      return -1;
    }
      if(bytesWaiting != bytesThisTransfer){
	ThrowError("RoachBackend:: Fail in call to recv");
	stopLoop = 1;
        return -1;
      }
      //      CTOUT("bytesthis transf, tot bytes: " << bytesThisTransfer << " , " << bytesReceived);
      lptr += bytesThisTransfer;
      bytesReceived += bytesThisTransfer;
      
      if(bytesReceived >= command.numBytesExpected_-1){
	stopLoop = 1;
      }
      
      /*
       * Check how long we've taken so far
       */
      stop.setToCurrentTime();
      //      CTOUT("start, stop: " << start << " , " << stop);
      diff = stop - start;
      //      COUT("Time taken so far: " << diff.getTimeInMicroSeconds());
      
      if(stopLoop==0){
	if(diff.getTimeInMicroSeconds() > ROACH_TIMEOUT_USEC){
	  stopLoop = 1;
	  return 2;
	};
      };

    }; // done with decent part of loop
  } while(stopLoop==0);

  /*
   * If we're taking too long, exit the program
   */
  
  //  COUT("Message Transfer complete in: " << diff.getTimeInMicroSeconds() << " total bytes read: " << bytesReceived);
  /*
   * NULL terminate the line
   */
  *(lptr++) = '\0';
  
  /*
   * Command this message to our command object, so we can refer to it later
   */
  if(command.numBytesExpected_ < 100){
    strcpy(command.responseReceived_, (char*) line);
  } else { 
    memcpy(&command.packet_, line, bytesReceived);
    command.packetPtr_ = &command.packet_;
  }
  return 1;
}


/***************************************************
 * Issue command and get response
 */

int RoachBackend::issueCommand(RoachBackendMsg& command)
{
  if(!roachIsConnected() || fd_ < 0)
    return 0;

  int status;
#if(0)
  COUT("COMMAND TO SEND: " << command.messageToSend_);
  COUT("bytes to send: " << command.cmdSize_);
  COUT("bytes expected: " << command.numBytesExpected_);
#endif

  int readStatus = 0;
  bool tryRead = true;
  int numTimeOut = 0;
  while(tryRead){

    try{
      // first we write the string out
      sendCommand(command);
    } catch(...){
      CTOUT("did not send correct number of bytes -- will rety");
      readStatus=2;
    };


    try {
      readStatus = readResponse(command);
    } catch(...) {
      CTOUT("got an error to readResponse.  This means the connection is bad, and we should disconnect");
      ThrowError("got an error to readResponse.  This means the connection is bad, and we should disconnect");
      return -1;
    }

    switch(readStatus){
    case 1:
      tryRead = false;
      switch(command.request_){
      case RoachBackendMsg::READ_DATA:
	//	COUT("TRYING TO PARSE BACKEND PACKET");
	parseBackendDataPacket(command);
	break;

      case RoachBackendMsg::ROACH_COMMAND:
	//	COUT("TRYING TO PARSE REGULAR COMMAND");
	parseBackendResponse(command);
	command.responseValid_ = true;
	break;
      };
      
      if(!command.responseValid_) {
	ThrowError("Response from Roach backend invalid");
	return 0;
      }
      break;
      
    case 2:
      numTimeOut++;
      COUT("increasing timeout rto , about to reissue: " << numTimeOut);
      break;
      
    case 0:
      numTimeOut++;
      COUT("increasing timeout rto , about to reissue: " << numTimeOut);
      break;
      
    case -1:
      // broken pipe
      ThrowError("broken pipe");
      break;

    }

    if(numTimeOut>5){
      ThrowError("Roach Port timed out too many times");
    }
  };


  return 1;
};



  



/***************************************************
 * Parse the response
 */

int RoachBackend::parseBackendDataPacket(RoachBackendMsg& command)
{
  if(!roachIsConnected() || fd_ < 0)
    return 0;

  // now we need to write the response as the collection of floats it
  // should be
  try {
    command.packetizeNetworkMsg();
    if(command.numFrames_ !=10) {
      command.responseValid_ = false;
    } else {
      command.responseValid_ = true;
    }
  } catch(...) {
    command.responseValid_ = false;
  };
  
  // that's it
  return 1;

}


/***************************************************
 * Parse the response
 */

int RoachBackend::parseBackendResponse(RoachBackendMsg& command)
{
  if(!roachIsConnected() || fd_ < 0)
    return 0;

  int retVal;
  // now we need to write the response as the collection of floats it
  // should be. 
  checkOneOutput(command);

  if(!command.responseValid_){
    COUT("Invalid response from the Roach");
    return 0;
  }

  if(!command.responseValue_){
    COUT("communication with Roach ok, but it didn't do what we wanted it to");
    return 1;
  };

  return 1;
}


/**.......................................................................
 * Check if box output is valid, and what its value is.
 */
void RoachBackend::checkOneOutput(RoachBackendMsg& command)
{

  std::string respStr(command.responseReceived_);
  String responseString(respStr);
  String expectedString(command.messageToSend_);

  String prefix = responseString.findNextStringSeparatedByChars(", ");
  String prefixExp = expectedString.findNextStringSeparatedByChars(", ");

    
  if(prefix.str() != prefixExp.str()){
    command.responseValid_ = false;
    command.responseGood_  = false;
  } else {
    command.responseValid_ = true;
    command.responseGood_  = (bool) responseString.findNextStringSeparatedByChars(", ").toFloat();
  };

  return;
};



/*.......................................................................
 * Get data from backend
*/

void RoachBackend::getData()
{

  int i,j,k;

  // check that we're connected.                                                                                                 
  if(!connected_){
    COUT("we're not connected.  not getting data");
    return;
  };

     // COUT("GetData");
  command_.packReadDataMsg();
  
  //  COUT("size of messageToSend: " << command_.cmdSize_);
  
  try {
    issueCommand(command_);
  } catch (...) {
    ThrowError("in getData: error in communication to roach board, should now disconnect");
    return;
  }

  if(!command_.responseValid_){
    ThrowError(" in getData: data from roach invalid.  should now disconnect");
    return;
  }
    
  // and next we put it into our ring buffer
  int numFrames = command_.numFrames_;
  double temp2;
  
  //  COUT("in RoachBackend::getData");
   // CTOUT("(NumFrames, version, intCount) : (" << numFrames << " , " << command_.version_ << " , " << command_.intCount_ << ")");

  if(numFrames!=10){
    COUT("WRONG NUMBER OF FRAMES RETURNED FROM THE ROACH BOARD-ignoring it for this time");
    COUT(" returned: "  << numFrames);
    missedCommCounter_++;
    return;
  }
  int test2;

  for (i=0;i<numFrames;i++) {

    // first the ones that are to be repeated by the number of frames
    version_[currentIndex_]    = command_.version_;
    packetSize_[currentIndex_] = command_.packetSize_;
    numFrames_[currentIndex_]  = command_.numFrames_;
    intCount_[currentIndex_]   = command_.intCount_; 
    tstart_[currentIndex_]     = command_.tstart_[i];
    switchstatus_[currentIndex_]     = command_.switchstatus_[i];
//    COUT(" tstartx: "  << tstart_[currentIndex_] <<"  " << temp2);
//    COUT(" temp2: "  << test2 <<"  "<<tstart_[currentIndex])
    tstop_[currentIndex_]      = command_.tstop_;
    intLength_[currentIndex_]  = command_.intLength_;
    mode_[currentIndex_]       = command_.mode_;
    res2_[currentIndex_]       = command_.res2_;
    bufferBacklog_[currentIndex_] = command_.bufferBacklog_[i]; 
    seconds_[currentIndex_] = command_.seconds_[i]; 
    useconds_[currentIndex_] = command_.useconds_[i]; 
    int thisTime = command_.tstart_[i];
  //  COUT("seconds_" << seconds_[currentIndex_]);
  //  COUT("bufferBacklog_" << bufferBacklog_[currentIndex_]);
     //   COUT("this time, index: " << thisTime << ", " << currentIndex_);

#if(0)  // 3d remnant
    for(j=0;j<NUM_CHANNELS_PER_BAND;j++){
      for(k=0;k<NUM_PARITY;k++){
	LL_[currentIndex_][j][k]  = command_.LL_[i][j][k];
	RR_[currentIndex_][j][k]  = command_.RR_[i][j][k];
	Q_[currentIndex_][j][k]   = command_.Q_[i][j][k]-65535;
	U_[currentIndex_][j][k]   = command_.U_[i][j][k]-65535;
	TL1_[currentIndex_][j][k] = command_.TL1_[i][j][k];
	TL2_[currentIndex_][j][k] = command_.TL2_[i][j][k];
      };
    };
#endif
    for(j=0;j<CHANNELS_PER_ROACH;j++){
      LL_[currentIndex_][j]  = command_.LL_[i][j];
      RR_[currentIndex_][j]  = command_.RR_[i][j];
      Q_[currentIndex_][j]   = command_.Q_[i][j]-65535;
      U_[currentIndex_][j]   = command_.U_[i][j]-65535;
      TL1_[currentIndex_][j] = command_.TL1_[i][j];
      TL2_[currentIndex_][j] = command_.TL2_[i][j];
    };
	for(j=0;j<32*8*2;j++){ //64 channels, four channels, real and imaginary- this is implemented on the roach as 32 even channels even and 32 odd channels-> the four channels now become 8 channels and we still have real an imaginary 
 		Coeffs_[j]= command_.Coeffs_[j]-65535;
	}
#if(0) 
    COUT(" LL_: "  << LL_[currentIndex_][32] << "switchStat " << switchstatus_[currentIndex_] << "tstart_ " << tstart_[currentIndex_]);
#endif
    // check that we're on to a new second                                              
    if(thisTime < prevTime_) {
     // COUT("thisTime, prevTime " << thisTime << "  ," << prevTime_);
      prevSecEnd_   = currentIndex_ - 1;
      if(prevSecEnd_ < 0)
	prevSecEnd_ = (RING_BUFFER_LENGTH-1);  // it's an index, starting from zero.     
      prevSecStart_ = thisSecStart_;
      thisSecStart_ = currentIndex_;
      //      COUT("IN ROACHBACKEND getData, start end:  " <<prevSecStart_ << ", " << prevSecEnd_);
    };
    // set the previous time
    prevTime_ = thisTime;

    // increment the currentIndex;                                                      
    currentIndex_++;
    if(currentIndex_ == RING_BUFFER_LENGTH) {
      currentIndex_ = 0;
    };
  };
  //COUT("IN GETDATA, CURRENT INDEX: " << currentIndex_);

  return;
}



/**.......................................................................                
 * Write the data to disk                                                                 
 */
void RoachBackend::writeData3D(gcp::util::TimeVal& currTime)
{
  int i,j, index;
  float thisMeanVal;
  float thisAvg;
  int numSamples = prevSecEnd_ - prevSecStart_ + 1;
  if(numSamples<0){
    numSamples += RING_BUFFER_LENGTH;
  };
  gcp::util::TimeVal dataTime;
  gcp::util::TimeVal offsetTime;
  gcp::util::TimeVal start, stop, diff;
  start.setToCurrentTime();
  
  //  COUT("prevSecStart, prevSecEnd: " << prevSecStart_ << " , " << prevSecEnd_);
  
  if(!share_){
    // nothing to do if no share object 
    return;
  }

     // COUT("WRITE DATA 3d");
#if 1
  if(numSamples > 100) {
    ReportError("Backend sent numSamples = " << numSamples);
    return;
  }
#endif


  // now all we have to do is write the data registers for the utc and 
  // the actual values from prevSecStart to prevSecStop;

  static float LLdataVals[CHANNELS_PER_ROACH][RECEIVER_SAMPLES_PER_FRAME];
  static float LLdataVector[CHANNELS_PER_ROACH*RECEIVER_SAMPLES_PER_FRAME];
  static float RRdataVals[CHANNELS_PER_ROACH][RECEIVER_SAMPLES_PER_FRAME];
  static float RRdataVector[CHANNELS_PER_ROACH*RECEIVER_SAMPLES_PER_FRAME];
  static float QdataVals[CHANNELS_PER_ROACH][RECEIVER_SAMPLES_PER_FRAME];
  static float QdataVector[CHANNELS_PER_ROACH*RECEIVER_SAMPLES_PER_FRAME];
  static float UdataVals[CHANNELS_PER_ROACH][RECEIVER_SAMPLES_PER_FRAME];
  static float UdataVector[CHANNELS_PER_ROACH*RECEIVER_SAMPLES_PER_FRAME];
  static float TL1dataVals[CHANNELS_PER_ROACH][RECEIVER_SAMPLES_PER_FRAME];
  static float TL1dataVector[CHANNELS_PER_ROACH*RECEIVER_SAMPLES_PER_FRAME];
  static float TL2dataVals[CHANNELS_PER_ROACH][RECEIVER_SAMPLES_PER_FRAME];
  static float TL2dataVector[CHANNELS_PER_ROACH*RECEIVER_SAMPLES_PER_FRAME];

  static std::vector<RegDate::Data> receiverUtc(RECEIVER_SAMPLES_PER_FRAME);
  static std::vector<float> switchFloatVector(RECEIVER_SAMPLES_PER_FRAME);
  static std::vector<float> seconds(RECEIVER_SAMPLES_PER_FRAME);
  static std::vector<float> useconds(RECEIVER_SAMPLES_PER_FRAME);
  static std::vector<float> buffBacklog(RECEIVER_SAMPLES_PER_FRAME);
  float meanVersion = 0;
  float meanIntCount = 0;
  float meanIntLength = 0;
  float meanMode = 0;

  for(i=0;i<RECEIVER_SAMPLES_PER_FRAME;i++){
		switchFloatVector[i]=switchstatus_[i];
		seconds[i]=float(seconds_[i]-1300000000);
		buffBacklog[i]=bufferBacklog_[i];
		   // COUT(" switchStat22: "  << switchstatus_[i]);
		COUT(" secondsReadin: "  << seconds[i]);
	//	COUT("switch i = "<<test[i]);
	}	
  // we're writing data from the previous second 
  
    try{
      // first we write the string out
  offsetTime.setSeconds(1);
  currTime -= offsetTime;
  offsetTime.setSeconds(0.3);
  currTime -= offsetTime;
    } catch(...) {
      COUT("Error with timing in Roach");
    }
  
  // FIRST LET'S DO THE AVERAGE VALUES
  index = prevSecStart_;
  //  COUT(" WRITING SMAPLES: " << numSamples);
  for (i=0;i<numSamples;i++){
    dataTime = currTime;
    
    //    dataTime.incrementSeconds(tstart_[index);
    //    dataTime.incrementSeconds(i*intLength_[index]); // if we do intlength, we have to do intlength/4*128/250000000
    dataTime.incrementSeconds(i*0.01);
    receiverUtc[i] = dataTime;
    
    meanVersion   += version_[index];
    meanIntCount  += intCount_[index];
    meanIntLength += intLength_[index];
    meanMode      += mode_[index];
    
    index++;
    if(index >= RING_BUFFER_LENGTH){
      index = 0;
    }
  }

  meanVersion   /= numSamples;
  meanIntCount  /= numSamples;
  meanIntLength /= numSamples;
  meanMode      /= numSamples;

  // write those out
  share_->writeReg(roachUtc_, &receiverUtc[0]);
  //COUT("writeReg "<<test[10]);
  share_->writeReg(roachSwitchStatus_, &switchFloatVector[0]);
  share_->writeReg(roachBufferBacklog_, &buffBacklog[0]);
  share_->writeReg(roachNTPSeconds_, &seconds[0]);
  share_->writeReg(roachNTPuSeconds_, &useconds[0]);
  share_->writeReg(roachVersion_, meanVersion);
  share_->writeReg(roachCount_, meanIntCount);
  share_->writeReg(roachIntLength_, meanIntLength);
  share_->writeReg(roachMode_, meanMode);
  
  
  // next we write the data
  index = prevSecStart_;

#if(0)
  static float LLtestVals[RECEIVER_SAMPLES_PER_FRAME][CHANNELS_PER_ROACH];
  static float LLtestVals2[CHANNELS_PER_ROACH][RECEIVER_SAMPLES_PER_FRAME];

  for(i=0;i<RECEIVER_SAMPLES_PER_FRAME;i++){
    for (j<0;j<NUM_CHANNELS_PER_BAND; j++){
      LLtestVals[i][2*j] = LL_[index][j][0];
      LLtestVals[i][2*j+1] = LL_[index][j][1];
    };
    index++;
    if(index >= RING_BUFFER_LENGTH){
      index = 0;
    };
  };

  for (i=0;i<numSamples;i++){
    for (j=0;j<NUM_CHANNELS_PER_BAND;j++){
      LLtestVals2[j][i] = LLtestVals[i][j];
    };
  };
   

  // next we write the data
  index = prevSecStart_;

  // put 3D vector into 2D
  for (i=0;i<numSamples;i++){
    for (j=0;j<NUM_CHANNELS_PER_BAND;j++){
      LLdataVals[2*j][i]   = LL_[index][j][0];
      LLdataVals[2*j+1][i] = LL_[index][j][1];

      RRdataVals[2*j][i]   = RR_[index][j][0];
      RRdataVals[2*j+1][i] = RR_[index][j][1];

      QdataVals[2*j][i]   = Q_[index][j][0];
      QdataVals[2*j+1][i] = Q_[index][j][1];

      UdataVals[2*j][i]   = U_[index][j][0];
      UdataVals[2*j+1][i] = U_[index][j][1];

      TL1dataVals[2*j][i]   = TL1_[index][j][0];
      TL1dataVals[2*j+1][i] = TL1_[index][j][1];

      TL2dataVals[2*j][i]   = TL2_[index][j][0];
      TL2dataVals[2*j+1][i] = TL2_[index][j][1];
    };

    index++;
    if(index >= RING_BUFFER_LENGTH){
      index = 0;
    }
  }
  // re-order the data registers into one long vector
  index = 0;
  for (i=0;i<CHANNELS_PER_ROACH;i++){
    for (j=0;j<RECEIVER_SAMPLES_PER_FRAME;j++){
      if(j<numSamples){
	LLdataVector[index]  = LLdataVals[i][j];
	//LLdataVector[index]  = LLtestVals2[i][j];
	RRdataVector[index]  = RRdataVals[i][j];
	QdataVector[index]   = QdataVals[i][j];
	UdataVector[index]   = UdataVals[i][j];
	TL1dataVector[index] = TL1dataVals[i][j];
	TL2dataVector[index] = TL2dataVals[i][j];
      } else {
	LLdataVector[index]  = 0;
	RRdataVector[index]  = 0;
	QdataVector[index]   = 0; 
	UdataVector[index]   = 0;
	TL1dataVector[index] = 0;
	TL2dataVector[index] = 0;
      };
      // increment the index.
      index++;
    };
  };

  // write the register
  share_->writeReg(roachLL_,  &LLdataVector[0]);
  share_->writeReg(roachRR_,  &RRdataVector[0]);
  share_->writeReg(roachQ_,   &QdataVector[0]);
  share_->writeReg(roachU_,   &UdataVector[0]);
  share_->writeReg(roachTL1_, &TL1dataVector[0]);
  share_->writeReg(roachTL2_, &TL2dataVector[0]);

  stop.setToCurrentTime();
    try{
	  diff = stop - start;
      // first we write the string out
    } catch(...) {
      COUT("Error with timing in Roach");
    }
    //  COUT("write data took seconds " << diff.getTimeInSeconds());
#endif
//reset the missed comm counter
	missedCommCounter_=0; 
  return;
}


/**.......................................................................                
 * Write the data to disk                                                                 
 */
void RoachBackend::writeData(gcp::util::TimeVal& currTime)
{
  //  CTOUT("about tto write data");
  int i,j, index;
  float thisMeanVal;
  float thisAvg;
  
  gcp::util::TimeVal dataTime;
  gcp::util::TimeVal offsetTime;
  gcp::util::TimeVal start, stop, diff;
  start.setToCurrentTime();
  
  //  COUT("IN ROACHBACKEND writeData, start end:  " <<prevSecStart_ << ", " << prevSecEnd_);

 // COUT("prevSecStart, prevSecEnd: " << prevSecStart_ << " , " << prevSecEnd_);
 // COUT("DEBUG STUFF");

    //  COUT("WRITE DATA ");
  int numSamples = prevSecEnd_ - prevSecStart_ + 1;
  
  if(numSamples<0){
    numSamples += RING_BUFFER_LENGTH;
  };
  if(!share_){
    // nothing to do if no share object 
    return;
  }

#if(1)
  if(numSamples > 101) {
    ReportError("Backend sent numSamples = " << numSamples);
    return;
  if(numSamples > 100){
	numSamples=100;
	ReportError("Rounding numSamples to 100. Backend sent numSamples = " << numSamples);
	}
  }
#endif

  


  // now all we have to do is write the data registers for the utc and 
  // the actual values from prevSecStart to prevSecStop;

  static float LLdataVals[CHANNELS_PER_ROACH][RECEIVER_SAMPLES_PER_FRAME];
  static float LLdataVector[CHANNELS_PER_ROACH*RECEIVER_SAMPLES_PER_FRAME];
  static float RRdataVals[CHANNELS_PER_ROACH][RECEIVER_SAMPLES_PER_FRAME];
  static float RRdataVector[CHANNELS_PER_ROACH*RECEIVER_SAMPLES_PER_FRAME];
  static float QdataVals[CHANNELS_PER_ROACH][RECEIVER_SAMPLES_PER_FRAME];
  static float QdataVector[CHANNELS_PER_ROACH*RECEIVER_SAMPLES_PER_FRAME];
  static float UdataVals[CHANNELS_PER_ROACH][RECEIVER_SAMPLES_PER_FRAME];
  static float UdataVector[CHANNELS_PER_ROACH*RECEIVER_SAMPLES_PER_FRAME];
  static float TL1dataVals[CHANNELS_PER_ROACH][RECEIVER_SAMPLES_PER_FRAME];
  static float TL1dataVector[CHANNELS_PER_ROACH*RECEIVER_SAMPLES_PER_FRAME];
  static float TL2dataVals[CHANNELS_PER_ROACH][RECEIVER_SAMPLES_PER_FRAME];
  static float TL2dataVector[CHANNELS_PER_ROACH*RECEIVER_SAMPLES_PER_FRAME];

  // next the time average values
  static float LLtimeAvg[CHANNELS_PER_ROACH];
  static float RRtimeAvg[CHANNELS_PER_ROACH];
  static float QtimeAvg[CHANNELS_PER_ROACH];
  static float UtimeAvg[CHANNELS_PER_ROACH];
  static float TL1timeAvg[CHANNELS_PER_ROACH];
  static float TL2timeAvg[CHANNELS_PER_ROACH];

  // next the freq average values;
  static float LLfreqAvg[RECEIVER_SAMPLES_PER_FRAME];
  static float RRfreqAvg[RECEIVER_SAMPLES_PER_FRAME];
  static float QfreqAvg[RECEIVER_SAMPLES_PER_FRAME];
  static float UfreqAvg[RECEIVER_SAMPLES_PER_FRAME];
  static float TL1freqAvg[RECEIVER_SAMPLES_PER_FRAME];
  static float TL2freqAvg[RECEIVER_SAMPLES_PER_FRAME];

  static float seconds[RECEIVER_SAMPLES_PER_FRAME];
  static float useconds[RECEIVER_SAMPLES_PER_FRAME];
  static float fpgaClockStamp[RECEIVER_SAMPLES_PER_FRAME];
  // frequency register
  static float channel[CHANNELS_PER_ROACH];
  
  static float buffBacklog[RECEIVER_SAMPLES_PER_FRAME];
  static float Coeffs[CHANNELS_PER_ROACH*4*2];

  // zero our average containers
  for (i=0;i<CHANNELS_PER_ROACH;i++){
    LLtimeAvg[i]  = 0;
    RRtimeAvg[i]  = 0;
    QtimeAvg[i]   = 0;
    UtimeAvg[i]   = 0;
    TL1timeAvg[i] = 0;
    TL2timeAvg[i] = 0;
    Coeffs[i]=0;
    // set the channel
    channel[i] = i;

  };
  for (i=0;i<RECEIVER_SAMPLES_PER_FRAME;i++){
    LLfreqAvg[i]  = 0;
    RRfreqAvg[i]  = 0;
    QfreqAvg[i]   = 0;
    UfreqAvg[i]   = 0;
    TL1freqAvg[i] = 0;
    TL2freqAvg[i] = 0;
  };

  static std::vector<RegDate::Data> receiverUtc(RECEIVER_SAMPLES_PER_FRAME);
  static std::vector<float> switchFloatVector(RECEIVER_SAMPLES_PER_FRAME);

  int temp;
  float meanVersion = 0;
  float meanIntCount = 0;
  float meanIntLength = 0;
  float meanMode = 0;
  float secondPart = 0;

  index=prevSecStart_;
  for(i=0;i<(numSamples);i++){
		switchFloatVector[i]=0;
		switchFloatVector[i]=switchstatus_[index];
		seconds[i]     = seconds_[index];
		fpgaClockStamp[i]  = tstart_[index];
		useconds[i] = useconds_[index];
		buffBacklog[i]=0;	
		buffBacklog[i]=bufferBacklog_[index];

//
#if(0) 
  COUT("Numsamples" << numSamples <<"Current_Index_" << index << " LL_: "  << LL_[index][32] << "switchStat " << switchstatus_[index] << "tstart_ " << tstart_[index]);
#endif
//    COUT(" switchStat22: "  << switchstatus_[index] << "switchFloatVector" << switchFloatVector[i] <<"i" << i);
		index++;
	    if(index >= RING_BUFFER_LENGTH){
	      index = 0;
	    }
//    COUT(" i=: "  << i);
		//COUT("test i = "<<switchFloatVector[i]);
	}	
  // we're writing data from the previous second 

  offsetTime.setSeconds(1);
  currTime -= offsetTime;
  // next two lines not needed:  we are using 0 in teh second spot.
  //  offsetTime.setSeconds(0.3);
  //  currTime -= offsetTime;
  
  // FIRST LET'S DO THE SINGLE VALUE AVERAGES
  index = prevSecStart_;
  //  COUT(" WRITING SMAPLES: " << numSamples);
  for (i=0;i<numSamples;i++){
    dataTime = currTime;
    
    
    secondPart = fpgaClockStamp[i]/250000000.;
    secondPart -= 0.005;
    // dataTime.incrementSeconds( [tstart_[index]);
    //    dataTime.incrementSeconds(i*intLength_[index]); // if we do intlength, we have to do intlength/4*128/250000000
    dataTime.incrementSeconds(secondPart);
    receiverUtc[i] = dataTime;
    //switchStatus[i] = switchstatus_[i];
    
    meanVersion   += version_[index];
    meanIntCount  += intCount_[index];
    meanIntLength += intLength_[index];
    meanMode      += mode_[index];
    
    index++;
    if(index >= RING_BUFFER_LENGTH){
      index = 0;
    }
  }

  meanVersion   /= numSamples;
  meanIntCount  /= numSamples;
  meanIntLength /= numSamples;
  meanMode      /= numSamples;

  // write those out
  share_->writeReg(roachUtc_, &receiverUtc[0]);
  share_->writeReg(roachSwitchStatus_, &switchFloatVector[0]);
  share_->writeReg(roachVersion_, meanVersion);
  share_->writeReg(roachCount_, meanIntCount);
  share_->writeReg(roachIntLength_, meanIntLength);
  share_->writeReg(roachMode_, meanMode);
  share_->writeReg(roachChan_, &channel[0]);
  share_->writeReg(roachNTPSeconds_, &seconds[0]);
  share_->writeReg(roachBufferBacklog_, &buffBacklog[0]);
  share_->writeReg(roachNTPuSeconds_, &useconds[0]);
  share_->writeReg(roachFPGAClockStamp_, &fpgaClockStamp[0]);
  // next the  averages:
  // first the frequency
#if 0
  // sjcm:  this might be messed up, but I don't see how
  index = prevSecStart_;
  for(j=0;j<CHANNELS_PER_ROACH;j++){
    for (i=0;i<RECEIVER_SAMPLES_PER_FRAME;i++){
      LLfreqAvg[i]  += (LL_[index][j]/CHANNELS_PER_ROACH);
      RRfreqAvg[i]  += (RR_[index][j]/CHANNELS_PER_ROACH);
      QfreqAvg[i]   += (Q_[index][j]/CHANNELS_PER_ROACH);
      UfreqAvg[i]   += (U_[index][j]/CHANNELS_PER_ROACH);
      TL1freqAvg[i] += (TL1_[index][j]/CHANNELS_PER_ROACH);
      TL2freqAvg[i] += (TL2_[index][j]/CHANNELS_PER_ROACH);
    }
    index++;
    if(index >= RING_BUFFER_LENGTH){
      index = 0;
    }
  };

#endif // try this instead.
  index = prevSecStart_;  
  for (i=0;i<RECEIVER_SAMPLES_PER_FRAME;i++){
    for(j=0;j<CHANNELS_PER_ROACH;j++){
      LLfreqAvg[i]  += (LL_[index][j]/( (float) (CHANNELS_PER_ROACH) ));
      RRfreqAvg[i]  += (RR_[index][j]/( (float) (CHANNELS_PER_ROACH) ));
      QfreqAvg[i]   += (Q_[index][j]/( (float) (CHANNELS_PER_ROACH) ));
      UfreqAvg[i]   += (U_[index][j]/( (float) (CHANNELS_PER_ROACH) ));
      TL1freqAvg[i] += (TL1_[index][j]/( (float) (CHANNELS_PER_ROACH) ));
      TL2freqAvg[i] += (TL2_[index][j]/( (float) (CHANNELS_PER_ROACH) ));
    }
    index++;
    if(index >= RING_BUFFER_LENGTH){
      index = 0;
    }
  };
  


  // now the time
  for (i=0;i<CHANNELS_PER_ROACH;i++){
    index = prevSecStart_;
    for(j=0;j<RECEIVER_SAMPLES_PER_FRAME;j++){
      LLtimeAvg[i]  += (LL_[index][i]/(float)RECEIVER_SAMPLES_PER_FRAME);
      RRtimeAvg[i]  += (RR_[index][i]/(float)RECEIVER_SAMPLES_PER_FRAME);
      QtimeAvg[i]   += (Q_[index][i]/(float)RECEIVER_SAMPLES_PER_FRAME);
      UtimeAvg[i]   += (U_[index][i]/(float)RECEIVER_SAMPLES_PER_FRAME);
      TL1timeAvg[i] += (TL1_[index][i]/(float)RECEIVER_SAMPLES_PER_FRAME);
      TL2timeAvg[i] += (TL2_[index][i]/(float)RECEIVER_SAMPLES_PER_FRAME);
      index++;
      if(index >= RING_BUFFER_LENGTH){
	index = 0;
      }
    }
  };

  for(i=0;i<=511;i++){

      Coeffs[i] = Coeffs_[i];
     // Coeffs[i] -=5000; //subtract off 10 000 which is used from the roach side to avoid any negative numbers being sent over network 
}

  // write the registers
  share_->writeReg(roachLLfreq_,  &LLfreqAvg[0]);
  share_->writeReg(roachRRfreq_,  &RRfreqAvg[0]);
  share_->writeReg(roachQfreq_,   &QfreqAvg[0]);
  share_->writeReg(roachUfreq_,   &UfreqAvg[0]);
  share_->writeReg(roachTL1freq_, &TL1freqAvg[0]);
  share_->writeReg(roachTL2freq_, &TL2freqAvg[0]);
  share_->writeReg(roachLLtime_,  &LLtimeAvg[0]);
  share_->writeReg(roachRRtime_,  &RRtimeAvg[0]);
  share_->writeReg(roachQtime_,   &QtimeAvg[0]);
  share_->writeReg(roachUtime_,   &UtimeAvg[0]);
  share_->writeReg(roachTL1time_, &TL1timeAvg[0]);
  share_->writeReg(roachTL2time_, &TL2timeAvg[0]);
  share_->writeReg(roachCoffs_, &Coeffs[0]);


  // next we write the data
  index = prevSecStart_;
  // re-order
  for (i=0;i<numSamples;i++){
    for (j=0;j<CHANNELS_PER_ROACH;j++){
      LLdataVals[j][i]  = LL_[index][j];
      RRdataVals[j][i]  = RR_[index][j];
      QdataVals[j][i]   = Q_[index][j];
      UdataVals[j][i]   = U_[index][j];
      TL1dataVals[j][i] = TL1_[index][j];
      TL2dataVals[j][i] = TL2_[index][j];
    };
    
    //    COUT("IN WRITE DATA: LLdataVals[0][" << i << "] = " << LLdataVals[0][i]);


    index++;
    if(index >= RING_BUFFER_LENGTH){
      index = 0;
    }
  }
  // re-order the data registers into one long vector
  index = 0;
  for (i=0;i<CHANNELS_PER_ROACH;i++){
    for (j=0;j<RECEIVER_SAMPLES_PER_FRAME;j++){
      if(j<numSamples){
	LLdataVector[index]  = LLdataVals[i][j];
	RRdataVector[index]  = RRdataVals[i][j];
	QdataVector[index]   = QdataVals[i][j];
	UdataVector[index]   = UdataVals[i][j];
	TL1dataVector[index] = TL1dataVals[i][j];
	TL2dataVector[index] = TL2dataVals[i][j];
      } else {
	LLdataVector[index]  = 0;
	RRdataVector[index]  = 0;
	QdataVector[index]   = 0; 
	UdataVector[index]   = 0;
	TL1dataVector[index] = 0;
	TL2dataVector[index] = 0;
      };
      // increment the index.
      index++;
    };
  };

  // write the register
  share_->writeReg(roachLL_,  &LLdataVector[0]);
  share_->writeReg(roachRR_,  &RRdataVector[0]);
  share_->writeReg(roachQ_,   &QdataVector[0]);
  share_->writeReg(roachU_,   &UdataVector[0]);
  share_->writeReg(roachTL1_, &TL1dataVector[0]);
  share_->writeReg(roachTL2_, &TL2dataVector[0]);

  stop.setToCurrentTime();
  //  COUT("write data start stop: " << start << " , " << stop);
  diff = stop - start;
  //  COUT("write data took seconds " << diff.getTimeInSeconds());

//reset the missed comm counter
	missedCommCounter_=0; 

  return;
}



/*.......................................................................
 * Get data from backend
*/

void RoachBackend::sendMessage(RoachBackendMsg command)
{
  // check that we're connected.                                                                                                 
  if(!connected_){
    return;
  };

  try {
    issueCommand(command_);
  } catch (...) {
    ThrowError("in sendMessage: error in communication to roach board, should now disconnect");
    return;
  }

  if(!command_.responseValid_){
    ThrowError("in sendMessage: Error getting data from Roach Board, should now disconnect");
  }
    
  return;
}

/**.......................................................................
 * Assigns our 3D vector memory so we don't have to do in the constructor definition
 */

void RoachBackend::Assign3DRingMemory()
{

  // Set up sizes.
  version_.resize(RING_BUFFER_LENGTH);
  packetSize_.resize(RING_BUFFER_LENGTH);
  bufferBacklog_.resize(RING_BUFFER_LENGTH);
  numFrames_.resize(RING_BUFFER_LENGTH);
  intCount_.resize(RING_BUFFER_LENGTH);
  tstart_.resize(RING_BUFFER_LENGTH);
  switchstatus_.resize(RING_BUFFER_LENGTH);
  seconds_.resize(RING_BUFFER_LENGTH);
  tstop_.resize(RING_BUFFER_LENGTH);
  intLength_.resize(RING_BUFFER_LENGTH);
  mode_.resize(RING_BUFFER_LENGTH);
  res2_.resize(RING_BUFFER_LENGTH);
  Coeffs_.resize(32*8*2);

#if(0)
  LL_.resize(RING_BUFFER_LENGTH);
  for (int i = 0; i < RING_BUFFER_LENGTH; ++i) {
    LL_[i].resize(NUM_CHANNELS_PER_BAND);
    for (int j = 0; j < NUM_CHANNELS_PER_BAND; ++j)
      LL_[i][j].resize(NUM_PARITY);
  }

  RR_.resize(RING_BUFFER_LENGTH);
  for (int i = 0; i < RING_BUFFER_LENGTH; ++i) {
    RR_[i].resize(NUM_CHANNELS_PER_BAND);
    for (int j = 0; j < NUM_CHANNELS_PER_BAND; ++j)
      RR_[i][j].resize(NUM_PARITY);
  }

  Q_.resize(RING_BUFFER_LENGTH);
  for (int i = 0; i < RING_BUFFER_LENGTH; ++i) {
    Q_[i].resize(NUM_CHANNELS_PER_BAND);
    for (int j = 0; j < NUM_CHANNELS_PER_BAND; ++j)
      Q_[i][j].resize(NUM_PARITY);
  }

  U_.resize(RING_BUFFER_LENGTH);
  for (int i = 0; i < RING_BUFFER_LENGTH; ++i) {
    U_[i].resize(NUM_CHANNELS_PER_BAND);
    for (int j = 0; j < NUM_CHANNELS_PER_BAND; ++j)
      U_[i][j].resize(NUM_PARITY);
  }

  TL1_.resize(RING_BUFFER_LENGTH);
  for (int i = 0; i < RING_BUFFER_LENGTH; ++i) {
    TL1_[i].resize(NUM_CHANNELS_PER_BAND);
    for (int j = 0; j < NUM_CHANNELS_PER_BAND; ++j)
      TL1_[i][j].resize(NUM_PARITY);
  }

  TL2_.resize(RING_BUFFER_LENGTH);
  for (int i = 0; i < RING_BUFFER_LENGTH; ++i) {
    TL2_[i].resize(NUM_CHANNELS_PER_BAND);
    for (int j = 0; j < NUM_CHANNELS_PER_BAND; ++j)
      TL2_[i][j].resize(NUM_PARITY);
  }
#endif

};



/**.......................................................................
 * Assigns our 3D vector memory so we don't have to do in the constructor definition
 */

void RoachBackend::Assign2DRingMemory()
{

  // Set up sizes.
  version_.resize(RING_BUFFER_LENGTH);
  bufferBacklog_.resize(RING_BUFFER_LENGTH);
  packetSize_.resize(RING_BUFFER_LENGTH);
  numFrames_.resize(RING_BUFFER_LENGTH);
  intCount_.resize(RING_BUFFER_LENGTH);
  tstart_.resize(RING_BUFFER_LENGTH);
  switchstatus_.resize(RING_BUFFER_LENGTH);
  seconds_.resize(RING_BUFFER_LENGTH);
  useconds_.resize(RING_BUFFER_LENGTH);
  tstop_.resize(RING_BUFFER_LENGTH);
  intLength_.resize(RING_BUFFER_LENGTH);
  mode_.resize(RING_BUFFER_LENGTH);
  res2_.resize(RING_BUFFER_LENGTH);

  Coeffs_.resize(32*8*2);

  LL_.resize(RING_BUFFER_LENGTH);
  for (int i = 0; i < RING_BUFFER_LENGTH; ++i) {
    LL_[i].resize(CHANNELS_PER_ROACH);
  }

  RR_.resize(RING_BUFFER_LENGTH);
  for (int i = 0; i < RING_BUFFER_LENGTH; ++i) {
    RR_[i].resize(CHANNELS_PER_ROACH);
  }

  Q_.resize(RING_BUFFER_LENGTH);
  for (int i = 0; i < RING_BUFFER_LENGTH; ++i) {
    Q_[i].resize(CHANNELS_PER_ROACH);
  }

  U_.resize(RING_BUFFER_LENGTH);
  for (int i = 0; i < RING_BUFFER_LENGTH; ++i) {
    U_[i].resize(CHANNELS_PER_ROACH);
  }

  TL1_.resize(RING_BUFFER_LENGTH);
  for (int i = 0; i < RING_BUFFER_LENGTH; ++i) {
    TL1_[i].resize(CHANNELS_PER_ROACH);
  }

  TL2_.resize(RING_BUFFER_LENGTH);
  for (int i = 0; i < RING_BUFFER_LENGTH; ++i) {
    TL2_[i].resize(CHANNELS_PER_ROACH);
  }


};

