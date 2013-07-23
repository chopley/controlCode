#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <string>

#include "gcp/control/code/unix/libunix_src/common/tcpip.h"

#include "gcp/util/common/Angle.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/util/common/DlpUsbThermal.h"
#include <fcntl.h>

# define USB_TIMEOUT_USEC 200000
# define SELECT_TIMEOUT_USEC 200000
//# define DEF_TERM_USB_PORT "/dev/tty.usbserial-DPRUKU0H"
//# define DEF_TERM_USB_PORT "/dev/ttyUSB1"
# define DEF_TERM_USB_PORT "/dev/ttyDlp"

// someting of note: the device works best if not queried
// continuously.  give it a 200 ms break between queries and it's
// great.

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructor 
 */
DlpUsbThermal::DlpUsbThermal()
{
  fd_          = -1;
  connected_   = false;
}

/**.......................................................................
 * Destructor
 */
DlpUsbThermal::~DlpUsbThermal()
{
  // Disconnect from the port
  disconnect();
}

/**.......................................................................
 * Connect to the port
 */
bool DlpUsbThermal::connect()
{

  // Return immediately if we are already connected.

  if(connected_)
    return true;

  struct termios termio;

  // open our port

  COUT("Attempting to open port: " << DEF_TERM_USB_PORT);
  fd_ = open(DEF_TERM_USB_PORT, O_RDWR|O_NONBLOCK, 0644);

  if(fd_ < 0) {
    ReportError("Error opening device");
    return false;
  }

  /*
   * Store the current state for restoration when we exit.
   */
  if(tcgetattr(fd_,&termioSave_) < 0) {
    ReportError("Error reading current port settings.");
    return false;
  }

  // Not sure I need this: 
  // 115200 BAUD, NO PARITY, 8 BITS, 1 STOP

  termio = termioSave_;

  // set up the comms
  // terminal input control
  termio.c_iflag = 0;
  termio.c_iflag |= IGNBRK;  // Ignore break on input
  termio.c_iflag |= IXON|IXOFF;

  // how system treats output
  termio.c_oflag = 0;

  // hardware control of terminal
  termio.c_cflag = 0;
  termio.c_cflag |=  CREAD; // Enable receiver
  termio.c_cflag |=    CS8; // 8-bit characters
  termio.c_cflag |= B38400; // where we've switched 38400 to be 115200
  termio.c_cflag &= ~PARENB; // Disable parity bit generation
  termio.c_cflag &= ~CSTOPB; // if this flag is set, 2 stop bits are sent
  termio.c_cflag |= CLOCAL; // Ignore modem control lines

  // various other terminal functions
  termio.c_lflag = 0;

  long vdisable;
  vdisable = fpathconf(fd_, _PC_VDISABLE);
  termio.c_cc[VINTR] = vdisable;

  /*
   * Set input/output speed
   */
  // for BaudRate of 115200, the speed is B115200
  if(cfsetispeed(&termio, B115200) < 0 || cfsetospeed(&termio,B115200) < 0) {
    fprintf(stderr,"Error setting port speed.\n");
    return false;
  }

  /*
   * Set the new attributes.
   */
  if(tcsetattr(fd_, TCSAFLUSH, &termio) < 0) {
    fprintf(stderr,"Error setting port.\n");
    return false;
  }

  DBPRINT(true, Debug::DEBUG31, "Connected to the Dlp Device");
  connected_  = true;

  /* send a first message to ensure we're talking */

  struct timespec delay;
  delay.tv_sec  =         0;
  delay.tv_nsec = 500000000;
  nanosleep(&delay, 0);
  writeString(";");

  // set output units to Celcius
  setOutputUnits(1);
  nanosleep(&delay, 0);
  setOutputUnits(1);

  // And add the file descriptor to the set to be watched for
  // readability and exceptions

  fdSet_.registerReadFd(fd_);
  fdSet_.registerExceptionFd(fd_);

  return true;
}

/**.......................................................................
 * Disconnect from port
 */
void DlpUsbThermal::disconnect()
{
  // Before we are connected, the fd will be initialized to -1

  if(fd_ >= 0) {

    if(close(fd_) < 0) {
      ReportSysError("In close()");
    };
  };

  // And set these to indicate we are disconnected.
  
  fd_        = -1;
  connected_ = false;
}


/**.......................................................................
 * Return true if we are connected to the usb device.
 */
bool DlpUsbThermal::isConnected()
{
  return connected_;
}


/**.......................................................................
 * write a message to the port
 */
int DlpUsbThermal::writeString(std::string message)
{
  int bytesWritten = 0;
  int len = message.size();

  //  CTOUT("message to send is: " << message << " length:" << len );


  if(fd_>0){
    bytesWritten = write(fd_, (const char*)message.c_str(), len);
  };

  //  CTOUT("bytes written: " << bytesWritten);

  return bytesWritten;
}


/**.......................................................................
 * Read bytes from the serial port and save the response
 */
int DlpUsbThermal::readPort(DlpUsbThermalMsg& msg)
{
  if(!isConnected() || fd_ < 0)
    return 0;

  int i,nbyte,waserr=0,nread=0;
  unsigned char line[DATA_MAX_LEN], *lptr=NULL;
  int ioctl_state=0;
  bool stopLoop = 0;
  TimeVal start, stop, diff;

  // Start checking how long this is taking.

  start.setToCurrentTime();

  // Set the line pointer pointing to the head of the line

  lptr = line;

  // See how many bytes are waiting to be read

  do {
    waitForResponse();
    ioctl_state=ioctl(fd_, FIONREAD, &nbyte);

#ifdef linux_i486_gcc
    waserr |= ioctl_state != 0;
#else
    waserr |= ioctl_state < 0;
#endif

    if(waserr) {
      ThrowError("IO control device Error");
    };

    /*
     * If a non-zero number of bytes was found, read them
     *
     if(nbyte > 0) {

     /*
     * Read the bytes one at a time
     */
    for(i=0;i < nbyte;i++) {
      if(read(fd_, lptr, 1) < 0) {
        COUT("readPort: Read error.\n");
        return 0;
      };

      if(*lptr != '\r' && nread < DATA_MAX_LEN) {
        lptr++;
        nread++;
      } else {
	// check if there are any more bytes on the port.  if there
	// are, discard the previous value and re-read
	ioctl_state=ioctl(fd_, FIONREAD, &nbyte);
	if (nbyte==0) {
	  stopLoop = 1;
	} else {
	  COUT("bytes still on buffer.  re-read");
	  stopLoop = 0;
	  lptr = line;
	  nread = 0;
	};
      };

    };

    /*
     * Check how long we've taken so far
     */
    stop.setToCurrentTime();
    diff = stop - start;
    
    //    COUT("time difference:  " << diff.getTimeInMicroSeconds());
    /*
     * If we're taking too long, exit the program
     */
    if(diff.getTimeInMicroSeconds() > USB_TIMEOUT_USEC){
      stopLoop = 1;
      return 2;
    };
  } while(stopLoop==0);

  /*
   * NULL terminate the line and print to stdout
   */
  *(lptr++) = '\0';

  /*
   * Forward the line to connected clients
   */
  strcpy(msg.responseReceived_, (char*) line);

  return 1;
}

/**.......................................................................
 * Read bytes from the serial port and save the response
 */
int DlpUsbThermal::readPort(DlpUsbThermalMsg& msg, bool withQ)
{
  if(!isConnected() || fd_ < 0)
    return 0;

  int i,nbyte,waserr=0,nread=0;
  unsigned char line[DATA_MAX_LEN], *lptr=NULL;
  int ioctl_state=0;
  bool stopLoop = 0;
  TimeVal start, stop, diff;

  // Start checking how long this is taking.

  start.setToCurrentTime();

  // Set the line pointer pointing to the head of the line

  lptr = line;

  // See how many bytes are waiting to be read

  do {
    waitForResponse();
    ioctl_state=ioctl(fd_, FIONREAD, &nbyte);

#ifdef linux_i486_gcc
    waserr |= ioctl_state != 0;
#else
    waserr |= ioctl_state < 0;
#endif

    if(waserr) {
      ThrowError("IO control device Error");
    };

    /*
     * If a non-zero number of bytes was found, read them
     *
     if(nbyte > 0) {

     /*
     * Read the bytes one at a time
     */
    for(i=0;i < nbyte;i++) {
      if(read(fd_, lptr, 1) < 0) {
        COUT("readPort: Read error.\n");
        return 0;
      };
      #if(0)
      // for debugging purposes, print out what we get
      print_bits(*lptr);

      if(isalnum(*lptr) || isprint(*lptr)) {
	COUT(*lptr);
      } else if(*lptr == '\r') {
	COUT("CR");
	//	lprintf(term, true ,stdout, " CR\n");
      } else if(*lptr == '\n') {
	COUT("\n");
	//	lprintf(term, true, stdout, " LF\n");
      } else if(*lptr == '\0') {
	COUT("NULL");
      } else {
	COUT(*lptr);
      }
      #endif

      if(*lptr != 'Q' && nread < DATA_MAX_LEN) {
        lptr++;
        nread++;
      } else {
	stopLoop = 1;
	};
    };

    /*
     * Check how long we've taken so far
     */
    stop.setToCurrentTime();
    diff = stop - start;
    
    //    COUT("time difference:  " << diff.getTimeInMicroSeconds());
    /*
     * If we're taking too long, exit the program
     */
    if(diff.getTimeInMicroSeconds() > USB_TIMEOUT_USEC && stopLoop==0){
      stopLoop = 1;
      return 2;
    };
  } while(stopLoop==0);

  /*
   * NULL terminate the line and print to stdout
   */
  *(lptr++) = '\0';

  /*
   * Forward the line to connected clients
   */
  strcpy(msg.responseReceived_, (char*) line);

  return 1;
}



/**.......................................................................
 * Wait for a response from the usb device
 */
void DlpUsbThermal::waitForResponse()
{
  TimeVal timeout(0, SELECT_TIMEOUT_USEC, 0);

  // Do nothing if we are not connected to the servo.

  if(!isConnected())
    return;

  // Now wait in select until the fd becomes readable, or we time out

  int nready = select(fdSet_.size(), fdSet_.readFdSet(), NULL, fdSet_.exceptionFdSet(), timeout.timeVal());

  // If select generated an error, throw it
  if(nready < 0) {
    ThrowSysError("In select(): ");
  } else if(nready==0) {
    ThrowError("Timed out in select");
  } else if(fdSet_.isSetInException(fd_)) {
    ThrowError("Exception occurred on file descriptor");
  }
}

/**.......................................................................
 * Packs our request into an object of class DlpUsbThermalMsg
 */
DlpUsbThermalMsg DlpUsbThermal::packCommand(DlpUsbThermalMsg::Request req, int input)
{

  DlpUsbThermalMsg msg;
  std::ostringstream os;

  msg.request_ = req;

  switch (req) {
  case DlpUsbThermalMsg:: SET_DATA_TYPE:
    msg.expectsResponse_ = false;
    switch (input) {
    case 0:  // wants binary
      os << "\\r"; 
      break;
      
    default:  // wants ascii
      os << "`\r"; 
      break;
    };
    break;
    
  case DlpUsbThermalMsg:: SET_UNITS:
    msg.expectsResponse_ = false;
    switch (input) {
    case 0:  // wants F
      os << "L\r"; 
      COUT("setting units: " << os.str());
      break;
      
    default:  // wants C
      os << ";\r"; 
      break;
    };
    break;

  case DlpUsbThermalMsg:: QUERY_TEMP:
    msg.expectsResponse_ = true;
    switch (input) {
    case 1:
      os << "9\r"; 
      break;

    case 2:
      os << "0\r"; 
      break;

    case 3:
      os << "-\r"; 
      break;

    case 4:
      os << "=\r"; 
      break;

    case 5:
      os << "O\r"; 
      break;

    case 6:
      os << "P\r"; 
      break;

    case 7:
      os << "[\r"; 
      break;

    case 8:
      os << "]\r"; 
      break;

    default:
      os << "9\r"; 
      break;
    };
    break;

  case DlpUsbThermalMsg:: QUERY_VOLTAGE:
    msg.expectsResponse_ = true;
    switch (input) {
    case 1:
      os << "Z\r"; 
      break;

    case 2:
      os << "X\r"; 
      break;

    case 3:
      os << "C\r"; 
      break;

    case 4:
      os << "V\r"; 
      break;

    case 5:
      os << "B\r"; 
      break;

    case 6:
      os << "N\r"; 
      break;

    case 7:
      os << "M\r"; 
      break;

    case 8:
      os << ",\r"; 
      break;

    default:
      os << "Z\r"; 
      break;
    };
    break;

  case DlpUsbThermalMsg:: QUERY_ALL_TEMP:
    msg.expectsResponse_ = true;    
    os << "90-=OP[]'\n";
    break;

  case DlpUsbThermalMsg:: QUERY_ALL_VOLT:
    msg.expectsResponse_ = true;    
    os << "ZXCVBNM,'";
    break;

  }
  msg.messageToSend_ = os.str();
  msg.cmdSize_ = msg.messageToSend_.size();
  
  return msg;
}

/**.......................................................................
 * Sends the command
 */
void DlpUsbThermal::sendCommand(DlpUsbThermalMsg& msg)
{
  // Don't try to send commands if we are not connected.

  if(!connected_)
    return;

  // Check that this is a valid command.

  if(msg.request_ == DlpUsbThermalMsg::INVALID)
    ThrowError("Sending the Dlp Thermal Module an invalid command.");

  int status = writeString(msg.messageToSend_);
  
  // writeString should return the number of bytes requested to be sent.

  if(status != msg.cmdSize_) {
    ThrowSysError("In writeString()");
  }
}

/**.......................................................................
 * Sends the command, reads the response, and parses the response
 */
DlpUsbThermalMsg DlpUsbThermal::issueCommand(DlpUsbThermalMsg::Request req, int input)
{
  // Set these up so that if we timeout, we try again.
  int numTimeOut = 0;
  int readStatus = 0;
  bool tryRead = 1;

  DlpUsbThermalMsg msg;

  // First we pack the command
  msg = packCommand(req, input);


  // If the msg expects a response, we want to ensure we try a couple of times before timing out.
  if(!msg.expectsResponse_)
    return msg;

  while(tryRead) {
    try { 
      // Next we send the command
      sendCommand(msg);
      readStatus = readPort(msg);
    } catch(...) {
      // there was an error waiting for response
      readStatus = 2;
    };

    switch(readStatus) {
    case 1:
      // It read things fine.  Check that it's valid before exiting
      parseResponse(msg);
      tryRead = 0;
      break;
    case 2:
      // went through a time out.  re-issue commands
      numTimeOut++;
      break;

    case 0:
      // not communicating:
      numTimeOut++;
      break;
    };
    if(numTimeOut>5){
      // There's an error on the port.  throw the error
      ThrowError(" Dlp Usb Port timeout on response");
    };
  };

  // and return the msg with all our info in it
  return msg;
}

/**.......................................................................
 * Sends the command, reads the response, and parses the response
 */
DlpUsbThermalMsg DlpUsbThermal::issueCommand(DlpUsbThermalMsg::Request req, int input, bool withQ)
{
  // Set these up so that if we timeout, we try again.
  int numTimeOut = 0;
  int readStatus = 0;
  bool tryRead = 1;

  DlpUsbThermalMsg msg;

  // First we pack the command
  msg = packCommand(req, input);


  // If the msg expects a response, we want to ensure we try a couple of times before timing out.
  if(!msg.expectsResponse_)
    return msg;

  while(tryRead) {
    try { 
      // Next we send the command
      sendCommand(msg);
      readStatus = readPort(msg, withQ);
    } catch(...) {
      // there was an error waiting for response
      readStatus = 2;
    };

    switch(readStatus) {
    case 1:
      // It read things fine.  Check that it's valid before exiting
      parseResponse(msg, withQ);
      tryRead = 0;
      break;
    case 2:
      // went through a time out.  re-issue commands
      numTimeOut++;
      break;

    case 0:
      // not communicating:
      numTimeOut++;
      break;
    };
    if(numTimeOut>5){
      // There's an error on the port.  throw the error
      ThrowError(" Dlp Usb Port timeout on response");
    };
  };

  # if(0)
  // and return the msg with all our info in it
  COUT("msg: " << msg.responseReceived_);

  int i;
  for (i=0;i<NUM_DLP_TEMP_SENSORS; i++) {
    COUT("value at exit of parseResponse: " << msg.responseValueVec_[i]);
  };
  #endif
  return msg;
}

 
/**.......................................................................
 * Parse the response -- takes the string and turns it into a float.
 */
void DlpUsbThermal::parseResponse(DlpUsbThermalMsg& msg) 
{

  // The string response is responseReceived_;
  std::string respStr(msg.responseReceived_);
  String responseString(respStr);

  msg.responseValue_ = responseString.findNextStringSeparatedByChars("VCF").toFloat();

  return;
};


/**.......................................................................
 * Parse the response -- takes the string and turns it into a float.
 */
void DlpUsbThermal::parseResponse(DlpUsbThermalMsg& msg, bool withQ) 
{

  // The string response is responseReceived_;
  std::string respStr(msg.responseReceived_);
  String responseString(respStr);
  int i;
  
  for (i=0;i<NUM_DLP_TEMP_SENSORS; i++) {
    msg.responseValueVec_[i] = responseString.findNextStringSeparatedByChars("VCFQ ").toFloat();
  };
  return;
};



/**.......................................................................
 * Specific Command Sets
 */

void DlpUsbThermal::setupDefault()
{

  // This command will just set up the return values as ASCII and the
  // Units in Centigrade.

  struct timespec delay;
  delay.tv_sec  =        0;
  delay.tv_nsec = 50000000;


  setOutputType(1);
  nanosleep(&delay, 0);  

  setOutputUnits(1);
  nanosleep(&delay, 0);  

  return;
};

void DlpUsbThermal::setOutputType(int outType)
{
  struct timespec delay;
  delay.tv_sec  =        0;
  delay.tv_nsec = 50000000;
  issueCommand(DlpUsbThermalMsg::SET_DATA_TYPE, outType);

  nanosleep(&delay, 0);  
}

void DlpUsbThermal::setOutputUnits(int outUnits)
{
  struct timespec delay;
  delay.tv_sec  =        0;
  delay.tv_nsec = 50000000;

  issueCommand(DlpUsbThermalMsg::SET_UNITS, outUnits);
  nanosleep(&delay, 0);  
}

float DlpUsbThermal::queryTemperature(int channel)
{
  DlpUsbThermalMsg msg;
  msg = issueCommand(DlpUsbThermalMsg::QUERY_TEMP, channel);
  return msg.responseValue_;
}

float DlpUsbThermal::queryVoltage(int channel)
{
  DlpUsbThermalMsg msg;
  msg = issueCommand(DlpUsbThermalMsg::QUERY_VOLTAGE, channel);
  return msg.responseValue_;
}

std::vector<float> DlpUsbThermal::queryAllTemps()
{
  DlpUsbThermalMsg msg;
  std::vector<float> temps(NUM_DLP_TEMP_SENSORS);
  msg = issueCommand(DlpUsbThermalMsg::QUERY_ALL_TEMP, 0, 0);

  int i;
  for (i=0;i<NUM_DLP_TEMP_SENSORS;i++){
    temps[i] = msg.responseValueVec_[i];
  };

  return temps;
};


std::vector<float> DlpUsbThermal::queryAllVoltages()
{
  int i;
  DlpUsbThermalMsg msg;
  std::vector<float> values(NUM_DLP_TEMP_SENSORS);
  msg = issueCommand(DlpUsbThermalMsg::QUERY_ALL_VOLT, 0, 0);

  for (i=0;i<NUM_DLP_TEMP_SENSORS;i++){
    values[i] = msg.responseValueVec_[i];
  };

  return values;
};




/*.......................................................................
 * Print the bits of a 1-byte unsigned char.
 */
void DlpUsbThermal::print_bits(unsigned char feature)
{
  unsigned result;
  int i;

  fprintf(stdout,"<");
  for(i=7;i >= 0;i--) {
    result = feature & (1U << i);
    fprintf(stdout, "%d",result ? 1 : 0);
  }
  fprintf(stdout,">");
  return;
}




