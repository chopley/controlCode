#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>


#include "gcp/control/code/unix/libunix_src/common/tcpip.h"

#include "gcp/util/common/Angle.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/antenna/control/specific/AntennaDrive.h"
#include "gcp/antenna/control/specific/AntennaException.h"
#include "gcp/antenna/control/specific/ServoComms.h"
#include "gcp/antenna/control/specific/SpecificShare.h"

#include <fcntl.h>

/*
 * Assign a default serial port device
 */
//#define DEF_TERM_SERIAL_PORT "/dev/tty.usbserial"
//#define DEF_TERM_SERIAL_PORT "/dev/ttyS0"
//#define DEF_TERM_SERIAL_PORT "/dev/ttyUSB2"
#define DEF_TERM_SERIAL_PORT "/dev/ttyServo"
#define TRACKING_LIMIT_ASEC 15

using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor with shared resources.
 */
ServoComms::ServoComms(SpecificShare* share, string name, bool sim) : 
  Board(share, name)
{
  sim_           =   sim;
  fd_            =    -1;
  connected_     = false;
  inactiveCount_ =   0;

  utc_         = 0;
  azPositions_ = 0;     
  azErrors_    = 0;        
  elPositions_ = 0;     
  elErrors_    = 0;        
  errorCount_  = 0;  
  slowAzPos_   = 0;  
  slowElPos_   = 0;  
  az1CommI_    = 0;  
  az2CommI_    = 0;  
  el1CommI_    = 0;  
  az1ActI_     = 0;  
  az2ActI_     = 0;  
  el1ActI_     = 0;  
  az1Enable_   = 0;  
  az2Enable_   = 0;  
  el1Enable_   = 0;  
  taskLoop_    = 0;
  encStatus_   = 0;
  lowSoftAz_   = 0;
  lowHardAz_   = 0;
  hiSoftAz_    = 0;
  hiHardAz_    = 0;
  lowSoftEl_   = 0;
  lowHardEl_   = 0;
  hiSoftEl_    = 0;
  hiHardEl_    = 0;
  azWrap_      = 0;
  azBrake_     = 0;
  elBrake_     = 0;
  eStop_       = 0;
  simulator_   = 0;
  ppsPresent_  = 0;


  utc_         =   findReg("utc");
  azPositions_ =   findReg("fast_az_pos");
  elPositions_ =   findReg("fast_el_pos");
  azErrors_    =   findReg("fast_az_err");
  elErrors_    =   findReg("fast_el_err");
  errorCount_  =   findReg("servo_error_count");
  slowAzPos_   =   findReg("slow_az_pos");
  slowElPos_   =   findReg("slow_el_pos");
  az1CommI_    =   findReg("command_current_az1");
  az2CommI_    =   findReg("command_current_az2");
  el1CommI_    =   findReg("command_current_el1");
  az1ActI_     =   findReg("actual_current_az1");
  az2ActI_     =   findReg("actual_current_az2");
  el1ActI_     =   findReg("actual_current_el1");
  az1Enable_   =   findReg("enable_status_az1");
  az2Enable_   =   findReg("enable_status_az2");
  el1Enable_   =   findReg("enable_status_el1");
  // ones for STS command
  taskLoop_    =   findReg("task_loop");               
  encStatus_   =   findReg("encoder_status");     
  lowSoftAz_   =   findReg("az_cw_soft_limit");       
  lowHardAz_   =   findReg("az_cw_hard_limit");       
  hiSoftAz_    =   findReg("az_ccw_soft_limit");        
  hiHardAz_    =   findReg("az_ccw_hard_limit");        
  lowSoftEl_   =   findReg("el_down_soft_limit");       
  lowHardEl_   =   findReg("el_down_hard_limit");       
  hiSoftEl_    =   findReg("el_up_soft_limit");        
  hiHardEl_    =   findReg("el_up_hard_limit");        
  azWrap_      =   findReg("az_no_wrap");
  azBrake_     =   findReg("az_brake_on");
  elBrake_     =   findReg("el_brake_on");
  eStop_       =   findReg("emer_stop_on");
  simulator_   =   findReg("simulator_running");
  ppsPresent_  =   findReg("pps_present");

  calibrationStatus_ = 0;
  initializationComplete_ = 0;
}
/**.......................................................................
 * Constructor with no shared resources.
 */
ServoComms::ServoComms() : Board()
{
  share_       = 0;
  fd_          = -1;
  connected_   = false;
}

/**.......................................................................
 * Destructor
 */
ServoComms::~ServoComms()
{
  // Disconnect from the SERVO port

  disconnect();
}

/**.......................................................................
 * Send a command to the servo.
 */
void ServoComms::sendCommand(ServoCommand& command)
{
  // Don't try to send commands if we are not connected.

  if(!connected_)
    return;

  // Check that this is a valid command.

  if(command.request_ == ServoCommand::INVALID)
    ThrowError("Sending the SERVO an invalid command.");

  //  COUT(command.messageToSend_);
  int status = writeString(command.messageToSend_);

  // writeString should return the number of bytes requested to be sent.
  
  if(status != command.cmdSize_) {
    ThrowSysError("In writeString()");
  }
}


/**.......................................................................
 * Private version to read a response into our internal message container.
 */
int ServoComms::readResponse(ServoCommand& command)
{
  // Check if we previously sent a valid command.
  
  if(command.request_ == ServoCommand::INVALID)
    ThrowError("Attempt to read a response before any command was sent.");

  // Complain if the command that was previously sent didn't expect a
  // response.
  
  if(!command.expectsResponse_)
    ThrowError("No reponse expected to the last sent command.");
    
  // Now we read the command from the serial port.

  return readSerialPort(command);
}

/**.......................................................................
 * issue the command, get response, check that it is valid.
 */

ServoCommand ServoComms::issueCommand(ServoCommand::Request req)
{
  std::vector<float> values;
  return issueCommand(req, values);
};

ServoCommand ServoComms::issueCommand(ServoCommand::Request req, std::vector<float>& values)
{
  int numTimeOut = 0;
  int readStatus = 0;
  bool tryRead = 1;

  ServoCommand command;
  command.packCommand(req, values);

  // If running in simulation mode, just return the command without
  // doing anything further
  if(sim_) {
    command.responseValid_ = 1;
    return command;
  }

  //  COUT("COMMAND TO BE ISSUED: " << command.messageToSend_);

  while(tryRead){
    try {
      sendCommand(command);
      //      COUT("COMMAND SENT: " << command.messageToSend_);
      readStatus = readResponse(command);
    } catch(...) {
      // there was en error in waitForResponse
      readStatus = 2;
    };
    
    switch(readStatus){
    case 1:
      // It read things fine.  Check that it's valid before exiting 
      command.interpretResponse();
      //COUT(command.responseReceived_);
      if(command.responseValid_==1){
	tryRead = 0;
	inactiveCount_ = 0;
      } else {
	numTimeOut++;
      };
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
      ThrowError(" Port timeout on response");
      inactiveCount_++;
    };
  };

  // and return the command class object with all our info in it 
  return command;
};

/**.......................................................................
 * Disconnect from the servo port.
 */
void ServoComms::disconnect()
{
  // Before we are connected, the fd will be initialized to -1

  // Note:  I am not reconfiguring the device back to its original state.

  if(fd_ >= 0) {

    tcsetattr(fd_, TCSAFLUSH, &termioSave_);

    if(close(fd_) < 0) {
      ReportSysError("In close()");
    }
  }

  // And set these to indicate we are disconnected.

  fd_        = -1;
  connected_ = false;
  inactiveCount_ = 0;
}

/**.......................................................................
 * Connect to the servo port.
 */
bool ServoComms::connect()
{
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

  struct termios termio;
  //  system("setserial /dev/ttyS0 spd_vhi");

  COUT("Attempting to open port: " << DEF_TERM_SERIAL_PORT << " at baudrate 115200");
  
  fd_ = open(DEF_TERM_SERIAL_PORT, O_RDWR|O_NONBLOCK, 0644);

  if(fd_ < 0) {
    ReportError("Error opening device");
    return false;
  }

  COUT("Connection established to Servo Box.");

  /*
   * Store the current state for restoration when we exit.
   */
  if(tcgetattr(fd_,&termioSave_) < 0) {
    ReportError("Error reading current port settings.");
    return false;
  }

  // FOR DOULOI, NEED 115200 BAUD, NO PARITY, 8 BITS, 1 STOP 

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

  DBPRINT(true, Debug::DEBUG31, "Connected to the SERVO");
  
  connected_  = true;
  
  /* send a first message to ensure we're talking */

  struct timespec delay;

  delay.tv_sec  =         0;
  delay.tv_nsec = 500000000;

  nanosleep(&delay, 0);

  writeString("XXX\r");

  nanosleep(&delay, 0);

  writeString("XXX\r");

  nanosleep(&delay, 0);

  // And add the file descriptor to the set to be watched for
  // readability and exceptions

  fdSet_.registerReadFd(fd_);
  fdSet_.registerExceptionFd(fd_);

  return true;
}

/**.......................................................................
 * Wait for a response from the servo
 */
void ServoComms::waitForResponse()
{
  TimeVal timeout(0, SERVO_TIMEOUT_USEC, 0);

  // Do nothing if we are not connected to the servo.

  if(!servoIsConnected())
    return;

  // Now wait in select until the fd becomes readable, or we time out

  int nready = select(fdSet_.size(), fdSet_.readFdSet(), NULL, fdSet_.exceptionFdSet(), timeout.timeVal());

  //  COUT("Dropped out of select with: nready = " << nready);

  // If select generated an error, throw it

  if(nready < 0) {
    ThrowSysError("In select(): ");
  } else if(nready==0) {
    ThrowError("Timed out in select");
    //   CTOUT("timed out in servo select: " << pthread_self());
  } else if(fdSet_.isSetInException(fd_)) {
    ThrowError("Exception occurred on file descriptor");
  }
  //  CTOUT("done with servo select: " << pthread_self());  
}

/**.......................................................................
 * Return true if we are connected to the servo.
 */
bool ServoComms::servoIsConnected()
{
  return connected_;
}

/**.......................................................................
 * write a message to the port
 */
int ServoComms::writeString(std::string message)
{
  int bytesWritten = 0;
  int len = message.size();

  //   CTOUT("message to send is: " << message << " length:" << len );
  

  if(fd_>0){
    bytesWritten = write(fd_, (const char*)message.c_str(), len);
  };

  return bytesWritten;
}


/**.......................................................................
 * Read bytes from the serial port and save the response
 */
int ServoComms::readSerialPort(ServoCommand& command)
{
  if(!servoIsConnected() || fd_ < 0)
    return 0;

  int i,nbyte,waserr=0,nread=0;
  unsigned char line[SERVO_DATA_MAX_LEN], *lptr=NULL;
  int ioctl_state=0;
  bool stopLoop = 0;
  TimeVal start, stop, diff;

  // Start checking how long this is taking.

  start.setToCurrentTime();

  // Set the line pointer pointing to the head of the line

  lptr = line;
  
  // See how many bytes are waiting to be read
  
  do {

    //    COUT("About to wait for response");
    waitForResponse();
    //    COUT("About to wait for response... done");

    ioctl_state=ioctl(fd_, FIONREAD, &nbyte);
    //    COUT("Found " << nbyte << " bytes MAX = " << SERVO_DATA_MAX_LEN);
    //    COUT("Found bytes: " << nbyte);
    
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
      //      COUT("About to read byte: " << i << " of " << nbyte << " " << pthread_self());
      if(read(fd_, lptr, 1) < 0) {
        // THROW ERRORS
        COUT("readSerialPort: Read error.\n");
	return 0;
      };
      
      if(*lptr != '\r' && nread < SERVO_DATA_MAX_LEN-1) {
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

    /*
     * If we're taking too long, exit the program
     */
    if(diff.getTimeInMicroSeconds() > SERVO_TIMEOUT_USEC){
      //      COUT("LINE READ: " << line);
      //  COUT("LAST ENTRY: " << (int)*lptr);
      // COUT("LAST ENTRY: " << (int)*(lptr-1));
      // COUT("LAST ENTRY: " << (int)*(lptr-2));
      // COUT("stop loop: " << stopLoop);
      // COUT("nread= " << nread);
      // COUT("Serial port timeout warning: " << diff.getTimeInMicroSeconds());
      stopLoop = 1;
      return 2;
    };
  } while(stopLoop==0);

  //  COUT("Serial complete in: " << diff.getTimeInMicroSeconds());  
  /*
   * NULL terminate the line and print to stdout
   */
  *(lptr++) = '\0';
  
  /*
   * Forward the line to connected clients
   */
  strcpy(command.responseReceived_, (char*) line);

  //  CTOUT("Exiting loop: line = " << line);
  return 1;
}

//------------------------------------------------------------
// Command set
//------------------------------------------------------------

/**............................................................
 *  setAzEl;
 */
void ServoComms::setAzEl(gcp::util::Angle& az, gcp::util::Angle& el)
{
  static std::vector<float> values(2);

  values[0] = az.degrees();
  values[1] = el.degrees();

  //  COUT("setting az/el to: " << az.degrees() << " , " << el.degrees());
  //  COUT("setting az/el to: " << values[0] << " , " << values[1]);

  
  issueCommand(ServoCommand::SEND_POS, values);
  
};

/**............................................................
 *  hardStopAntenna -- this command is pretty harsh on the drives, do
 *  not use it lightly.
 */
void ServoComms::hardStopAntenna()
{
  issueCommand(ServoCommand::STOP_ALL);
};

/**............................................................
 *  haltAntenna -- This command queries for the current antenna
 *  location and re-issues it to have the antenna stop.
 */
void ServoComms::haltAntenna()
{
  ServoCommand command;
  // query the current location 
  command = issueCommand(ServoCommand::GET_AZEL);
  
  // re-issue same location 
  std::vector<float> values(2);
  values[0] = command.responseValue_[0];
  values[1] = command.responseValue_[1];

  issueCommand(ServoCommand::SEND_POS, values);
};

/**............................................................
 *  Calibrate the encoders
 */
void ServoComms::calibrateEncoders()
{
  issueCommand(ServoCommand::CAL_ENCODERS);
};

/**............................................................
 *  Initialize the antenna
 */
void ServoComms::initializeAntenna(int antNum)
{
  //  if(sim_) {
  //  return;
  //}
  COUT("Initializing Antenna");
  
  // initilize the variable.
  initializationComplete_ = 0;

  /* antNum corresponds to the different load parameters on each
     antenna:
     0 == simulator
     1 == cbass
     2 == kube
  */

  // first we check the version number 
  ServoCommand command;

  command = issueCommand(ServoCommand::QUERY_VERSION);
  if(command.responseValid_!=1)
    {
      throw Error("ServoComms::initializeAntenna: Servo code version not recognized \n");
    };

  COUT("CHECKING AMPLIFIER STATUS");
  // Next we start the amplifiers 
  issueCommand(ServoCommand::START_AMP_COMMS);
  wait();

  // 7VDC command for no reason 
  writeString("7VDC");
  wait();
  
  // Check the amplifiers are communicating 
  issueCommand(ServoCommand::ALIVE_AZ_AMP1);
  wait();
  issueCommand(ServoCommand::ALIVE_AZ_AMP2);
  wait();
  issueCommand(ServoCommand::ALIVE_EL_AMP);
  wait();
  
  // Enable the amplifiers 
  issueCommand(ServoCommand::ENABLE_AZ_AMP1);
  wait();
  issueCommand(ServoCommand::ENABLE_AZ_AMP2);
  wait();
  issueCommand(ServoCommand::ENABLE_EL_AMP);
  wait();

  // Load the loop parameters 
  COUT("Loading Loop Parameters");
  std::vector<float> vals(37);
  switch(antNum){
  case 0:
    {
      const float pars[37] = {5.0,5.0,1.0,1.0,0.01,0.0,1.0,1.0,1.0,1.0,1.0,1.0,8.0,8.0,1.0,1.0,1.2,1.2,0.0,0.0,1.0,1.0,800,800,-800,-800,1.0,1.0,50,144000,144000,12000,12000,-43.987,67.135,2900,2900};
      for (unsigned i=0;i<37;i++){
	vals[i] = pars[i];
      };
    };
    break;
    
  case 1:
    {
      // adding 0.25 degrees in az, 0.23 in elevation.  went from -251.488 to -251.238, 102.301 to 102.531 -- right direction. yay.
       // changing the velocity limits and the max torque allowed
      //const float pars[37] = {0.4,0.4,2.0,2.0,0.01,0.01,1.5,1.0,5.0,1.0,1.0,2.0,2.0,1.2,1.0,1.0,0.8,1.5,0.0,0.0,1.0,1.0,800,1200,-800,-1600,1.0,1.0,150,144000,144000,20000,20000,-251.238,102.531,1522.5,19749};
      // removed and re-installed the encoder.
      const float pars[37] = {0.4,0.4,2.0,2.0,0.01,0.01,1.5,1.0,5.0,1.0,1.0,2.0,2.0,1.2,1.0,1.0,0.8,1.5,0.0,0.0,1.0,1.0,800,1200,-800,-1600,1.0,1.0,150,144000,144000,20000,20000,-35.638,102.531,1522.5,19749};
      for (unsigned i=0;i<37;i++){
	vals[i] = pars[i];
      };
    };
    break;
    
  case 2:
    {
      const float pars[37] = {0.4,0.4,2.0,2.0,0.01,0.01,1.5,1.0,2.5,1.0,1.0,5.0,2.0,1.2,1.0,1.0,0.8,1.5,0.0,0.0,1.0,1.0,800,1200,-800,-1300,1.0,1.0,150,144000,144000,20000,20000,-264.318,160.538,1522.5,19749};
      for (unsigned i=0;i<37;i++){
	vals[i] = pars[i];
      };
    };
    break;
  };
  //  COUT("Here 2");
  issueCommand(ServoCommand::LOAD_LOOP_PARAMS, vals);
  wait();

  //  COUT("Here 3");
  // stop all that's going on 
  issueCommand(ServoCommand::STOP_ALL);
  wait();
  
  // check the status to see if the encoders are calibrated 
  CTOUT("QUERYING STATUS");
  command = issueCommand(ServoCommand::QUERY_STATUS);
  wait();
  
  //  int currentValue = (int) command.responseValue_[13];
  //  calibrationStatus_ = currentValue;
  calibrationStatus_ = 0;

  // stop the antenna if for some reason it decides to start moving.
  issueCommand(ServoCommand::STOP_ALL);  
  wait();

  COUT("Here 4");
  if(calibrationStatus_ == 0){
    COUT("issuing calibration command ");
    // not calibrated, calibrate the damn thing.
    issueCommand(ServoCommand::CAL_ENCODERS);
  } else {
    // stop the antenna
    issueCommand(ServoCommand::STOP_ALL);
  };
  wait();
  return;
};

/**............................................................
 *  Query Status of Antenna and Amplifier and write to register map
 */
void ServoComms::queryStatus()
{
  ServoCommand command;

  // Status of Servo 
  command = issueCommand(ServoCommand::QUERY_STATUS);
  if(share_) {
    // write the appropriate responses to the register map
    recordStatusResponse(command);
  }  

  //  COUT("Here QS 1");
  // Error Count 
  command = issueCommand(ServoCommand::NUMBER_ERRORS);
  if(share_) {
    share_->writeReg(errorCount_, (int) command.responseValue_[0]);
  }  

  //  COUT("Here QS 2: share_ = " << share_);

  // Azimuth, Elevation 
  command = issueCommand(ServoCommand::GET_AZEL);
  if(share_) {
    share_->writeReg(slowAzPos_, command.responseValue_[0]);
    share_->writeReg(slowElPos_, command.responseValue_[1]);

    //    COUT("AZ response was: " << command.responseValue_[0]);
    //    COUT("EL response was: " << command.responseValue_[1]);
  }  

  //  COUT("Here QS 3");
  // Commanded Amplifier Currents 
  command = issueCommand(ServoCommand::COMMAND_CURR_AZ1);
  if(share_) {
    share_->writeReg(az1CommI_, command.responseValue_[0]);
  }  
  command = issueCommand(ServoCommand::COMMAND_CURR_AZ2);
  if(share_) {
    share_->writeReg(az2CommI_, command.responseValue_[0]);
  }  
  command = issueCommand(ServoCommand::COMMAND_CURR_EL);
  if(share_) {
    share_->writeReg(el1CommI_, command.responseValue_[0]);
  }  

  
  // Actual Amplifier Currents 
  command = issueCommand(ServoCommand::ACTUAL_CURR_AZ1);
  if(share_) {
    share_->writeReg(az1ActI_, command.responseValue_[0]);
  }  
  command = issueCommand(ServoCommand::ACTUAL_CURR_AZ2);
  if(share_) {
    share_->writeReg(az2ActI_, command.responseValue_[0]);
  }  
  command = issueCommand(ServoCommand::ACTUAL_CURR_EL);
  if(share_) {
    share_->writeReg(el1ActI_, command.responseValue_[0]);
  }  

  // Amplifier Enable Status 
  command = issueCommand(ServoCommand::STATUS_AZ_AMP1);
  if(share_) {
    share_->writeReg(az1Enable_, (bool) command.responseValue_[0]);
  }  
  command = issueCommand(ServoCommand::STATUS_AZ_AMP2);
  if(share_) {
    share_->writeReg(az2Enable_, (bool) command.responseValue_[0]);
  }  
  command = issueCommand(ServoCommand::STATUS_EL_AMP);
  if(share_) {
    share_->writeReg(el1Enable_, (bool) command.responseValue_[0]);
  }  


  return;

};

/**............................................................
 *  Finish the initialization
 */
void ServoComms::finishInitialization()
{
  COUT("Finishing the initialization");
  // Begin the tracking loops 
  issueCommand(ServoCommand::STOP_ALL);

  issueCommand(ServoCommand::BEGIN_TRACK_LOOP);

  issueCommand(ServoCommand::BEGIN_PPS_LOOP);

  // set the intitialization to complete
  initializationComplete_ = 1;
  COUT("Initialization Complete");
};


/**............................................................
 *  Query Status of Antenna and Amplifier and write to register map
 */
void ServoComms::queryStatus(gcp::util::TimeVal& currTime)
{
  ServoCommand command;

  // Status of Servo 
  command = issueCommand(ServoCommand::QUERY_STATUS);
  if(share_) {
    // write the appropriate responses to the register map
    recordStatusResponse(command);
  }  

  // Error Count 
  command = issueCommand(ServoCommand::NUMBER_ERRORS);
  if(share_) {
    share_->writeReg(errorCount_, (int) command.responseValue_[0]);
  }  

  // Azimuth, Elevation 
  command = issueCommand(ServoCommand::GET_AZEL);
  if(share_) {
    share_->writeReg(slowAzPos_, command.responseValue_[0]);
    share_->writeReg(slowElPos_, command.responseValue_[1]);

    //    COUT("AZ response was: " << command.responseValue_[0]);
    //    COUT("EL response was: " << command.responseValue_[1]);
  }  

  // Commanded Amplifier Currents 
  command = issueCommand(ServoCommand::COMMAND_CURR_AZ1);
  if(share_) {
    share_->writeReg(az1CommI_, command.responseValue_[0]);
  }  
  command = issueCommand(ServoCommand::COMMAND_CURR_AZ2);
  if(share_) {
    share_->writeReg(az2CommI_, command.responseValue_[0]);
  }  
  command = issueCommand(ServoCommand::COMMAND_CURR_EL);
  if(share_) {
    share_->writeReg(el1CommI_, command.responseValue_[0]);
  }  

  
  // Actual Amplifier Currents 
  command = issueCommand(ServoCommand::ACTUAL_CURR_AZ1);
  if(share_) {
    share_->writeReg(az1ActI_, command.responseValue_[0]);
  }  
  command = issueCommand(ServoCommand::ACTUAL_CURR_AZ2);
  if(share_) {
    share_->writeReg(az2ActI_, command.responseValue_[0]);
  }  
  command = issueCommand(ServoCommand::ACTUAL_CURR_EL);
  if(share_) {
    share_->writeReg(el1ActI_, command.responseValue_[0]);
  }  

  // Amplifier Enable Status 
  command = issueCommand(ServoCommand::STATUS_AZ_AMP1);
  if(share_) {
    share_->writeReg(az1Enable_, (bool) command.responseValue_[0]);
  }  
  command = issueCommand(ServoCommand::STATUS_AZ_AMP2);
  if(share_) {
    share_->writeReg(az2Enable_, (bool) command.responseValue_[0]);
  }  
  command = issueCommand(ServoCommand::STATUS_EL_AMP);
  if(share_) {
    share_->writeReg(el1Enable_, (bool) command.responseValue_[0]);
  }  

  // Fill the UTC register with appropriate time values
  fillUtc(currTime);

  return;

};

/**............................................................
 *  Query Antenna Locations from Previous Second
 */
void ServoComms::queryAntPositions()
{
  // Query the previous seconds' positions

  //  COUT("QUERYING POSITIONS");
  ServoCommand command = issueCommand(ServoCommand::GET_PRIOR_LOC);  
  //  COUT("done QUERYING POSITIONS");

  if(command.responseValueValid_) {
    // store the data
    static float azPos[SERVO_POSITION_SAMPLES_PER_FRAME];
    static float elPos[SERVO_POSITION_SAMPLES_PER_FRAME];
    static float azErr[SERVO_POSITION_SAMPLES_PER_FRAME];
    static float elErr[SERVO_POSITION_SAMPLES_PER_FRAME];
    
    for (unsigned i=0; i < 5; i++){
      azPos[i] = command.responseValue_[i*4+0];
      elPos[i] = command.responseValue_[i*4+1];
      azErr[i] = command.responseValue_[i*4+2];
      elErr[i] = command.responseValue_[i*4+3];
    };
    
    // Write them to shared memory

    if(share_) {
      share_->writeReg(azPositions_, azPos);
      share_->writeReg(elPositions_, elPos);
      share_->writeReg(azErrors_,    azErr);
      share_->writeReg(elErrors_,    elErr);
    }

    // Write them out as debug:
    //    COUT("AZ POS: " << azPos[0] << "," << azPos[1] << "," << azPos[2] << "," << azPos[3] << "," << azPos[4]);

  }
};

/**............................................................
 *  Query Antenna Locations from Previous Second
 */
void ServoComms::queryAntPositions(gcp::util::TimeVal& currTime)
{
  // Query the previous seconds' positions

  /*
  COUT("time of previous data");
  CTOUT("mjdays: " << currTime.getMjdDays());
  CTOUT("mjdSeconds: " << currTime.getMjdSeconds());
  CTOUT("mjdNanoSeconds: " << currTime.getMjdNanoSeconds());
  */
  ServoCommand command = issueCommand(ServoCommand::GET_PRIOR_LOC);  
  //  COUT("RESPO:  " << command.responseReceived_);

  if(command.responseValueValid_) {
    // store the data
    static float azPos[SERVO_POSITION_SAMPLES_PER_FRAME];
    static float elPos[SERVO_POSITION_SAMPLES_PER_FRAME];
    static float azErr[SERVO_POSITION_SAMPLES_PER_FRAME];
    static float elErr[SERVO_POSITION_SAMPLES_PER_FRAME];
    
    for (unsigned i=0; i < 5; i++){
      azPos[i] = command.responseValue_[i*4+0];
      elPos[i] = command.responseValue_[i*4+1];
      azErr[i] = command.responseValue_[i*4+2];
      elErr[i] = command.responseValue_[i*4+3];
    };
    
    // Write them to shared memory
    
    if(share_) {
      share_->writeReg(azPositions_, azPos);
      share_->writeReg(elPositions_, elPos);
      share_->writeReg(azErrors_,    azErr);
      share_->writeReg(elErrors_,    elErr);
      
      // Fill the UTC register with appropriate time values
      
      fillUtc(currTime);
    }

  };
};

/**.......................................................................
 * Fill the utc register with the time stamps appropriate for the data
 * just read back from the servo box
 */
void ServoComms::fillUtc(gcp::util::TimeVal& currTime)
{
  static std::vector<RegDate::Data> fastServoUtc(SERVO_POSITION_SAMPLES_PER_FRAME);

  // Time corresponding to the last 1pps tick is one second ago, on
  // the 1-second boundary

  currTime.incrementSeconds(-1.0);

  for(unsigned i=0; i < fastServoUtc.size(); i++) {
    fastServoUtc[i]  = currTime;
    fastServoUtc[i] += i * MS_PER_SERVO_POSITION_SAMPLE;
  }
  share_->writeReg(utc_, &fastServoUtc[0]);
}

/**.......................................................................
 * Tell the PMAC to read a new position
 */
void ServoComms::commandNewPosition(PmacTarget* pmac)
{
  // Stub to write the commanded position to the servo box

  gcp::util::Angle az = pmac->PmacAxis(Axis::AZ)->getAngle();
  gcp::util::Angle el = pmac->PmacAxis(Axis::EL)->getAngle();

  setAzEl(az, el);
}

/**.......................................................................
 * Read the previously obtained servo monitor data to update our view
 * of where the telescope axes are currently positioned.
 *
 * Returns a boolean: is the antenna tracking ok?
 */
bool ServoComms::readPosition(AxisPositions* axes, Model* model)
{
  // Read the last position returned.  This should be the position
  // commanded two ticks ago

  if(share_) {

    float azDegrees;
    float elDegrees;

    share_->readReg(slowAzPos_, &azDegrees);
    share_->readReg(slowElPos_, &elDegrees);

    Angle azAngle;
    Angle elAngle;
    
    azAngle.setDegrees(azDegrees);
    elAngle.setDegrees(elDegrees);
    
    axes->az_.topo_ = azAngle.radians();
    axes->el_.topo_ = elAngle.radians();


    // the return of this function is used to let the tracker thread
    // know if we're tracking the source.  so we have to query the
    // errors in the antenna locations (according to the servo), and
    // determine whether we are tracking.

    ServoCommand command;
    // query the current location 
    command = issueCommand(ServoCommand::POSITION_ERRORS);
    float azErr = command.responseValue_[0]*cos(elDegrees*3.14159/180);  // in mDeg
    float elErr = command.responseValue_[1];  // in mDeg
    float deltaErrAsec = sqrt( azErr*azErr + elErr*elErr)*60*60/1000;

    if(deltaErrAsec < TRACKING_LIMIT_ASEC){
      return true;
    } else {
      return false;
    }

  }
  return true;
}

bool ServoComms::isBusy()
{
  if(!servoIsConnected()){
    COUT("servo not connected, and therefore not busy");
    // we're not connected
    return false;
  }

  // The servo will be busy only during calibration of the encoders
  try {
    ServoCommand command = issueCommand(ServoCommand::QUERY_STATUS);
    calibrationStatus_ = (int) command.responseValue_[13];
  } catch(...) { 
    COUT("CAUGHT AN ERROR WHEN QUERYING STATUS");
  };

  //  COUT("CAL STATUS: " << calibrationStatus_ << " , init stat: " << initializationComplete_);
  if(calibrationStatus_ == 1){
    return true;
  } else if (calibrationStatus_==2 && initializationComplete_==0) {
    // encoders have been calibrated, but we need to issue the final
    // initialization commands

    finishInitialization();
    return true;
  } else {
    return false;
  };
}

unsigned ServoComms::readPositionFault()
{
  return 0;
}


/**............................................................
 *  Record the result from Status to the Register Map
 */
void ServoComms::recordStatusResponse(ServoCommand& command)
{
  // Function to record it to compartmentalize the loop
  int i;
  for (i=0; i<16; i++) {
    switch(i){
    case 0:
      share_->writeReg(taskLoop_, (int) command.responseValue_[i]);
      break;

    case 1:
      share_->writeReg(hiSoftEl_,  (bool) command.responseValue_[i]);
      break;
      
    case 2:
      share_->writeReg(lowSoftEl_, (bool) command.responseValue_[i]);
      break;
     
    case 3:
      share_->writeReg(hiHardEl_,  (bool) command.responseValue_[i]);
      break;

    case 4:
      share_->writeReg(lowHardEl_, (bool) command.responseValue_[i]);
      break;

    case 5:
      share_->writeReg(lowSoftAz_, (bool) command.responseValue_[i]);
      break;

    case 6:
      share_->writeReg(hiSoftAz_,  (bool) command.responseValue_[i]);
      break;

    case 7:
      share_->writeReg(lowHardAz_, (bool) command.responseValue_[i]);
      break;

    case 8:
      share_->writeReg(hiHardAz_,  (bool) command.responseValue_[i]);
      break;

    case 9:
      share_->writeReg(azWrap_,  (bool) command.responseValue_[i]);      
      break;

    case 10:
      share_->writeReg(azBrake_,  (bool) command.responseValue_[i]);      
      break;

    case 11:
      share_->writeReg(elBrake_,  (bool) command.responseValue_[i]);      
      break;

    case 12:
      share_->writeReg(eStop_,  (bool) command.responseValue_[i]);      
      break;      
      
    case 13:
      share_->writeReg(encStatus_, (int) command.responseValue_[i]);
      break;

    case 14:
      share_->writeReg(simulator_,  (bool) command.responseValue_[i]);
      break;

    case 15:
      share_->writeReg(ppsPresent_,  (bool) command.responseValue_[i]);
      break;

    };
  };
  return;
};

/**............................................................
 *  set maximum azimuth velocity
 */
void ServoComms::setMaxAzVel(int antNum, float vMax)
{
  // Here we just load the loop parameters but change the maximum azimuth velocity.

  // Load the loop parameters 
  std::vector<float> vals(37);
  switch(antNum){
  case 0:
    {
      const float pars[37] = {5.0,5.0,1.0,1.0,0.01,0.0,1.0,1.0,1.0,1.0,1.0,1.0,8.0,8.0,1.0,1.0,1.2,1.2,0.0,0.0,1.0,1.0,800,800,-800,-800,1.0,1.0,50,144000,144000,12000,12000,-43.987,67.135,2900,2900};
      for (unsigned i=0;i<37;i++){
	if (i==8){
	  vals[i] = vMax;
	} else {
	  vals[i] = pars[i];
	};
      };
    };
    break;
    
  case 1:
    {
      const float pars[37] = {0.4,0.4,2.0,2.0,0.01,0.01,1.5,1.0,5.0,1.2,1.0,5.0,2.0,1.2,1.0,1.0,0.8,1.5,0.0,0.0,1.0,1.0,800,1200,-800,-1600,1.0,1.0,150,144000,144000,20000,20000,-35.638,102.531,1522.5,19749};
      for (unsigned i=0;i<37;i++){
	if (i==8){
	  vals[i] = vMax;
	} else {
	  vals[i] = pars[i];
	};
      };
    };
    break;
    
  case 2:
    {
      const float pars[37] = {0.4,0.4,2.0,2.0,0.01,0.01,1.5,1.0,2.5,1.0,1.0,5.0,2.0,1.2,1.0,1.0,0.8,1.5,0.0,0.0,1.0,1.0,800,1200,-800,-1300,1.0,1.0,150,144000,144000,20000,20000,-264.318,160.538,1522.5,19749};
      for (unsigned i=0;i<37;i++){
	if (i==8){
	  vals[i] = vMax;
	} else {
	  vals[i] = pars[i];
	};
      };
    };
    break;
  };

  // Next we just issue the command
  issueCommand(ServoCommand::LOAD_LOOP_PARAMS, vals);

  return;
};
  

/**............................................................
 *  Check that the 1PPS is present
 */
bool ServoComms::isPpsPresent() {

  bool isPresent;
  share_->readReg(ppsPresent_, &isPresent);
  
  return isPresent;
};

/**............................................................
 *  Check if the antenna is calibrating
 */
bool ServoComms::isCalibrated() {

  int calStatus;
  bool isCalibrated;
  share_->readReg(encStatus_, &calStatus);
  
  isCalibrated = calStatus == 2;

  return isCalibrated;
};


/**............................................................
 *  Check that the proper tracking loop is running
 */
int ServoComms::checkTrackLoop() {

  int loopStatus;
  share_->readReg(taskLoop_, &loopStatus);
  
  return loopStatus;
};






/**............................................................
 *  Method that waits
 */
void ServoComms::wait(long nsec) {
  
  struct timespec delay;
  delay.tv_sec = 0;
  delay.tv_nsec = nsec;

  nanosleep(&delay, NULL);
}
  
