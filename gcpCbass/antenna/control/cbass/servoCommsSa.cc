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
#include "gcp/antenna/control/specific/ServoCommsSa.h"
#include "gcp/antenna/control/specific/SpecificShare.h"

#include <fcntl.h>

//Added by CJC 18 June 2010
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <cstdlib>
#include <cstring>
#include <unistd.h>
//extern "C" size_t strlen(const char*);

////////////////////////

/*
 * Assign a default serial port device
 */
#define TRACKING_LIMIT_ASEC 45
#define DEFAULT_PORT_NUMBER 1503
#define DEFAULT_CONTROLLER_NAME "c-bass2"


using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor with shared resources.
 */
ServoCommsSa::ServoCommsSa(SpecificShare* share, string name, bool sim) : 
  Board(share, name)
{
  sim_         =   sim;
  fd_          =    -1;
  connected_   = false;

  utc_        	         = 0;
  azPositions_           = 0;
  azErrors_              = 0;	    
  elPositions_           = 0;
  elErrors_              = 0;	    
  slowAzPos_  	         = 0;
  slowElPos_  	         = 0;
  servoStatus_  	 = 0;
  servoThermalCutouts_   = 0;
  servoContactors_       = 0;	    
  servoCircuitBreakers_  = 0;
  servoBrakes_  	 = 0;
  driveLids_ 	         = 0;
  azWrap_                = 0;

  utc_         =   findReg("utc");
  azPositions_ =   findReg("fast_az_pos");
  elPositions_ =   findReg("fast_el_pos");
  azErrors_    =   findReg("fast_az_err");
  elErrors_    =   findReg("fast_el_err");
  slowAzPos_   =   findReg("slow_az_pos");
  slowElPos_   =   findReg("slow_el_pos");
  // ones for STS command
  servoStatus_         = findReg("servo_status");     
  servoThermalCutouts_ = findReg("thermal_cutout");
  servoContactors_     = findReg("contactor");
  servoCircuitBreakers_= findReg("circuit_breaker");
  servoBrakes_         = findReg("mechanical_brakes");
  driveLids_           = findReg("drive_lids");
  azWrap_              = findReg("az_no_wrap");


  initializationPartOne_  = 0;
  initializationComplete_ = 0;
  alarmStatus_            = 0;
}
/**.......................................................................
 * Constructor with no shared resources.
 */
ServoCommsSa::ServoCommsSa() : Board()
{
  share_       = 0;
  fd_          = -1;
  connected_   = false;
}

/**.......................................................................
 * Destructor
 */
ServoCommsSa::~ServoCommsSa()
{
  // Disconnect from the SERVO port

  disconnect();
}

/**.......................................................................
 * Send a command to the servo.
 */
void ServoCommsSa::sendCommand(ServoCommandSa& command)
{
  // Don't try to send commands if we are not connected.
  if(!connected_)
    {
    COUT("not connected");
    return;
    }

  // Check that this is a valid command.

  if(command.request_ == ServoCommandSa::INVALID)
    ThrowError("Sending the SERVO an invalid command.");
  
  int status = writeString(command.messageToSend_);
  
  // writeString should return the number of bytes requested to be sent.
  
  if(status != command.cmdSize_) {
    ThrowSysError("In writeString()");
  }
}


/**.......................................................................
 * Private version to read a response into our internal message container.
 */
int ServoCommsSa::readResponse(ServoCommandSa& command)
{
  // Check if we previously sent a valid command.
  char buffer[256];
  int n;
  if(command.request_ == ServoCommandSa::INVALID)
    ThrowError("Attempt to read a response before any command was sent.");

  // Complain if the command that was previously sent didn't expect a
  // response.
  
  if(!command.expectsResponse_)
    ThrowError("No reponse expected to the last sent command.");
    
  return readTCPPort(command);

}

/**.......................................................................
 * issue the command, get response, check that it is valid.
 */

ServoCommandSa ServoCommsSa::issueCommand(ServoCommandSa::Request req)
{
  std::vector<float> values;
  return issueCommand(req, values);
};

ServoCommandSa ServoCommsSa::issueCommand(ServoCommandSa::Request req, std::vector<float>& values)
{
  int numTimeOut = 0;
  int readStatus = 0;
  bool tryRead = 1;

  ServoCommandSa command;
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
      readStatus = readResponse(command);
      // COUT("RESPONSE Received:  " << command.responseReceived_);
    } catch(...) {
      // there was en error in waitForResponse
      readStatus = 2;
    };
    
    switch(readStatus){
    case 1:
      // It read things fine.  Check that it's valid before exiting 
      command.interpretResponse();
      if(command.responseValid_==1){
	tryRead = 0;
	//	COUT("RESPONSE IS VALID:  " << command.responseReceived_);
      } else {
	numTimeOut++;
	//	COUT("RESPONSE IS inVALID:  " << command.responseReceived_);
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
      COUT("timed out more than five times");
      ThrowError(" Port timeout on response CJC");
    };
  };

  // quick break of 10ms
  wait(10000000);

  // and return the command class object with all our info in it 
  return command;
};

/**.......................................................................
 * Disconnect from the servo port.
 */
void ServoCommsSa::disconnect()
{
  // Before we are connected, the fd will be initialized to -1

  // Note:  I am not reconfiguring the device back to its original state.

  if(fd_ >= 0) {

    if(close(fd_) < 0) {
      ReportSysError("In close()");
    }
  }

  // And set these to indicate we are disconnected.

  fd_        = -1;
  connected_ = false;

}


/**.......................................................................
 * Connect to the servo port.
 */
bool ServoCommsSa::connect()
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
  
  struct termios termio;
  portno = DEFAULT_PORT_NUMBER;
  COUT("Attempting to open TCP Communication port: " << portno << " at");
  
  //CJC 18/6/2010
  
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_ < 0) {
    ReportError("Error opening TCP/IP Socket");
  }
  server = gethostbyname(DEFAULT_CONTROLLER_NAME);
  if (server == NULL) {
    fprintf(stderr,"ERROR, no such host TCP/IP\n");
    exit(0);
  }

  COUT("Connection established to Servo Box.");

  
  //CJC 18/6/2010
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
  serv_addr.sin_port = htons(portno);
  if (::connect(fd_,(const sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)    //if (connect(sockfd,&serv_addr,sizeof(serv_addr)) < 0) 
    ReportError("ERROR connecting");
  
  // printf("Please enter the message: ");
  bzero(buffer,256);
  
  DBPRINT(true, Debug::DEBUG31, "Connected to the SERVO");
  
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

  initializationPartOne_  = 0;
  initializationComplete_ = 0;

  return true;
}

/**.......................................................................
 * Wait for a response from the servo
 */
void ServoCommsSa::waitForResponse()
{
  TimeVal timeout(0, SERVO_TIMEOUT_USEC, 0);
  // Do nothing if we are not connected to the servo.

  if(!servoIsConnected()){
     COUT("Server is not connected\n");
    return;
  }

  // Now wait in select until the fd becomes readable, or we time out
  int nready = select(fdSet_.size(), fdSet_.readFdSet(), NULL, fdSet_.exceptionFdSet(), timeout.timeVal());
  
  //  COUT("Dropped out of select with: nready = " << nready);

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
 * Return true if we are connected to the servo.
 */
bool ServoCommsSa::servoIsConnected()
{
  return connected_;
}

/**.......................................................................
 * write a message to the port
 */
int ServoCommsSa::writeString(std::string message)
{
  int bytesWritten = 0;
  int len = message.size();

  if(fd_>0){
    //changed CJC
    //    COUT("message to send is: " << message);
    //    COUT(" length:" << len );
    bytesWritten = write(fd_, (const char*)message.c_str(), len);
  };

  return bytesWritten;
}


/***************************************************
 * Read the TCP port
 */

int ServoCommsSa::readTCPPort(ServoCommandSa& command)
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

    waitForResponse();

    ioctl_state=ioctl(fd_, FIONREAD, &nbyte);
    
    for(i=0;i < nbyte;i++) {
      if(read(fd_, lptr, 1) < 0) {
	COUT("readTCPPort: Read error.\n");
	return 0;
      };
      
      if(*lptr != '\r' && nread < SERVO_DATA_MAX_LEN-1) {
	//      if(*lptr != '\r' && nread < 25) {
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
      //COUT("LAST ENTRY: " << (int)*lptr);
      //COUT("LAST ENTRY: " << (int)*(lptr-1));
      //COUT("LAST ENTRY: " << (int)*(lptr-2));
      //COUT("stop loop: " << stopLoop);
      //COUT("nread= " << nread);
      //COUT("Serial port timeout warning: " << diff.getTimeInMicroSeconds());
      stopLoop = 1;
      return 2;
    };
  } while(stopLoop==0);
  
  //  COUT("Message Transfer complete in: " << diff.getTimeInMicroSeconds());  

  /*
   * NULL terminate the line and print to stdout
   */
  *(lptr++) = '\0';
  
  /*
   * Forward the line to connected clients
   */
  strcpy(command.responseReceived_, (char*) line);

  //    COUT("Exiting loop: line = " << line);
  return 1;
}
//------------------------------------------------------------
// Command set
//------------------------------------------------------------

/**............................................................
 *  setAzEl;
 */
void ServoCommsSa::setAzEl(gcp::util::Angle& az, gcp::util::Angle& el, gcp::util::Angle& az1, gcp::util::Angle& el1, gcp::util::Angle& az2, gcp::util::Angle& el2, gcp::util::TimeVal& mjd)
{
  static std::vector<float> values(8);

  mjd.setToCurrentTime();

  values[0] = az.degrees();
  values[1] = el.degrees();
  values[2] = az1.degrees();
  values[3] = el1.degrees();
  values[4] = az2.degrees();
  values[5] = el2.degrees();
  values[6] = (float) mjd.getMjdSeconds();
  mjd.incrementSeconds(2.0);
  values[7] = (float) mjd.getMjdSeconds();

  //  COUT("IN setAzEl, mjd: " << mjd.getMjdSeconds());


  //    COUT("setting az/el to: " << az.degrees() << " , " << el.degrees());
  //    COUT("setting az1/el1 to: " << az1.degrees() << " , " << el1.degrees());
  //    COUT("setting az2/el2 to: " << az2.degrees() << " , " << el2.degrees());
    //    COUT("setting t0/t2 to: " << values[6] << " , " << values[7]);

  //  COUT("setting az/el to: " << values[0] << " , " << values[1]);

  issueCommand(ServoCommandSa::SEND_POS_TRIP, values);
};

/**............................................................
 *  haltAntenna -- This command queries for the current antenna
 *  location and re-issues it to have the antenna stop.
 */
void ServoCommsSa::haltAntenna()
{
  ServoCommandSa command;
  // query the current location 
  command = issueCommand(ServoCommandSa::GET_AZEL);
  
  // re-issue same location 
  std::vector<float> values(2);
  values[0] = command.responseValue_[0];
  values[1] = command.responseValue_[1];

  issueCommand(ServoCommandSa::SEND_POS, values);
};

/**............................................................
 *  hardStopAntenna -- Disengages the drives
 */
void ServoCommsSa::hardStopAntenna()
{

  // re-issue same location 
  std::vector<float> values(1);
  values[0] = 0;

  issueCommand(ServoCommandSa::SERVO_ENGAGE, values);
};



/**............................................................
 *  Initialize the antenna
 */
void ServoCommsSa::initializeAntenna()
{

  if(sim_) {
    return;
  }
  COUT("Initializing Antenna");
  
  // initilize the variable.
  initializationComplete_ = false;
  initializationPartOne_  = false;

  ServoCommandSa command;

  // check the status to see if we need to enable the circuits
  COUT("Querying Status in Initialization");
  command = issueCommand(ServoCommandSa::QUERY_STATUS);
  COUT("response: " << command.responseReceived_);
  wait();

  /* check the status, if the az contactors are not on, turn them on.
     always turn the clutches on when we're done, make sure they're
     still engaged.
  */

  bool driveLidsOpen = (bool) command.responseValue_[17];
  std::vector<float> vals(7);

  // Check if the drive lids are open
  if(driveLidsOpen){
    // Do not do anything. 
    ReportMessage("Drive Lid is Open.  Disengaging the telescope");
    vals[0] = 0;
    issueCommand(ServoCommandSa::SERVO_ENGAGE, vals);
    
    return;
  }

  // if drive lids are not open, check the contactors

  float contactors = command.responseValue_[2] + command.responseValue_[6] + command.responseValue_[10] + command.responseValue_[14];
  if(contactors>0){
    ReportMessage("Engaging Contactors");
    issueCommand(ServoCommandSa::AZ_CONTACTORS_ON);
    issueCommand(ServoCommandSa::EL_CONTACTORS_ON);

    return;
  } else { 
    ReportMessage("Contactors Engaged");
  };

  // if contactors on, release the brakes

  float brakes = command.responseValue_[16];
  if(brakes>0){
    ReportMessage("Releasing Brakes");
    issueCommand(ServoCommandSa::AZ_BRAKE_OFF);
    issueCommand(ServoCommandSa::EL_BRAKE_OFF);

    return;
  } else { 
    ReportMessage("Brakes Released");
  };

  // if brakes released, turn on clutches
  ReportMessage("Engaging Clutches");
  issueCommand(ServoCommandSa::CLUTCHES_ON);

  // need to check if things are initialized properly
  wait();
  initializationPartOne_ = isPartOneComplete();

  return;
};

/**............................................................
 *  Query Status of Antenna and Amplifier and write to register map
 */
void ServoCommsSa::queryStatus()
{
  ServoCommandSa command;

  // Azimuth, Elevation 
  command = issueCommand(ServoCommandSa::GET_AZEL);
  if(share_) {
    share_->writeReg(slowAzPos_, command.responseValue_[0]);
    share_->writeReg(slowElPos_, command.responseValue_[1]);

    //    COUT("AZ response was: " << command.responseValue_[0]);
    //    COUT("EL response was: " << command.responseValue_[1]);
  }  

  // Status of Servo 
  // will probe it using specialized commands

  command = issueCommand(ServoCommandSa::QUERY_STATUS);

  // Set whether the alarm is on
  alarmStatus_ = (bool) command.responseValue_[19];

  if(share_) {
    // write the appropriate responses to the register map
    recordStatusResponse(command);
  }  

  return;

};


/**............................................................
 *  Query Status of Antenna and Amplifier and write to register map
 */
void ServoCommsSa::queryStatus(gcp::util::TimeVal& currTime)
{
  ServoCommandSa command;

  // Azimuth, Elevation 
  command = issueCommand(ServoCommandSa::GET_AZEL);
  if(share_) {
    share_->writeReg(slowAzPos_, command.responseValue_[0]);
    share_->writeReg(slowElPos_, command.responseValue_[1]);

    //    COUT("AZ response was: " << command.responseValue_[0]);
    //    COUT("EL response was: " << command.responseValue_[1]);
  }  

  // Status of Servo 
  command = issueCommand(ServoCommandSa::QUERY_STATUS);


  // Set whether the alarm is on
  alarmStatus_ = (bool) command.responseValue_[19];

  if(share_) {
    // write the appropriate responses to the register map
    recordStatusResponse(command);
  }  

  // Fill the UTC register with appropriate time values
  //  fillUtc(currTime);

  return;

};

/**............................................................
 *  Query Antenna Locations from Previous Second
 */
void ServoCommsSa::queryAntPositions()
{
  // Query the previous seconds' positions

  //  COUT("QUERYING POSITIONS");
  ServoCommandSa command = issueCommand(ServoCommandSa::GET_PRIOR_LOC);  
  //  COUT("done QUERYING POSITIONS");

  if(command.responseValueValid_) {
    // store the data
    static float azPos[SERVO_POSITION_SAMPLES_PER_FRAME];
    static float elPos[SERVO_POSITION_SAMPLES_PER_FRAME];
    static float azErr[SERVO_POSITION_SAMPLES_PER_FRAME];
    static float elErr[SERVO_POSITION_SAMPLES_PER_FRAME];
    
    for (unsigned i=0; i < 5; i++){
      azPos[i] = command.responseValue_[i];
      elPos[i] = command.responseValue_[i + 5];
      azErr[i] = command.responseValue_[i + 10]*1000; // in mdeg
      elErr[i] = command.responseValue_[i + 15]*1000; // in mdeg
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
void ServoCommsSa::queryAntPositions(gcp::util::TimeVal& currTime)
{
  // Query the previous seconds' positions
  /*
  COUT("time of previous data");
  CTOUT("mjdays: " << currTime.getMjdDays());
  CTOUT("mjdSeconds: " << currTime.getMjdSeconds());
  CTOUT("mjdNanoSeconds: " << currTime.getMjdNanoSeconds());
  */
  ServoCommandSa command = issueCommand(ServoCommandSa::GET_PRIOR_LOC);  

  if(command.responseValueValid_) {
    // store the data
    static float azPos[SERVO_POSITION_SAMPLES_PER_FRAME];
    static float elPos[SERVO_POSITION_SAMPLES_PER_FRAME];
    static float azErr[SERVO_POSITION_SAMPLES_PER_FRAME];
    static float elErr[SERVO_POSITION_SAMPLES_PER_FRAME];
    
    for (unsigned i=0; i < 5; i++){
      azPos[i] = command.responseValue_[i];
      elPos[i] = command.responseValue_[i + 5];
      azErr[i] = command.responseValue_[i + 10]*1000;  // in mdeg
      elErr[i] = command.responseValue_[i + 15]*1000;  // in mdeg
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
void ServoCommsSa::fillUtc(gcp::util::TimeVal& currTime)
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
void ServoCommsSa::commandNewPosition(PmacTarget* pmac, PmacTarget* pmac1, PmacTarget* pmac2, gcp::util::TimeVal& mjd)
{
  // Stub to write the commanded position to the servo box

  gcp::util::Angle az = pmac->PmacAxis(Axis::AZ)->getAngle();
  gcp::util::Angle el = pmac->PmacAxis(Axis::EL)->getAngle();
  gcp::util::Angle az1 = pmac1->PmacAxis(Axis::AZ)->getAngle();
  gcp::util::Angle el1 = pmac1->PmacAxis(Axis::EL)->getAngle();
  gcp::util::Angle az2 = pmac2->PmacAxis(Axis::AZ)->getAngle();
  gcp::util::Angle el2 = pmac2->PmacAxis(Axis::EL)->getAngle();

  //  COUT("IN comandnewposition, mjd: " << mjd.getMjdSeconds());

  setAzEl(az, el, az1, el1, az2, el2, mjd);
}

/**.......................................................................
 * Read the previously obtained servo monitor data to update our view
 * of where the telescope axes are currently positioned.
 *
 * Returns a boolean: is the antenna tracking ok?
 */
bool ServoCommsSa::readPosition(AxisPositions* axes, Model* model)
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

    ServoCommandSa command;
    // query the current location 
    command = issueCommand(ServoCommandSa::POSITION_ERRORS);
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

bool ServoCommsSa::isBusy()
{ 
  if(!servoIsConnected()){
    COUT("servo not connected, and therefore not busy");
    // we're not connected
    return false;
  }

  // The servo will be busy during startup if the initialization is not complete.
  if(initializationComplete_ == 0){
    // check that part is complete
    if(initializationPartOne_ == 0){
      COUT("Trying to initialize antenna again");
      initializeAntenna();
    } else {
      finishInitialization();
    }
    return true;
  }

  // The servo will also be busy if the alarm is going off
  if(alarmStatus_ == 1){
    ReportMessage("There's a whole lot of buzzing going on");
    return true;
  }
  
  // otherwise, we're not busy
  return false;
}

/* 
 * Check for first part of initialization
 */
bool ServoCommsSa::isPartOneComplete()
{

  if(!servoIsConnected()){
    COUT("servo not connected, and so initialziation not complete");
    // we're not connected
    return false;
  }
  bool complete = false;


  // check the status
  command_ = issueCommand(ServoCommandSa::QUERY_STATUS);
  wait();

  /* if drive lids closed, brakes not engaged, and contactors on,
     we're good */

  bool driveLidsOpen = (bool) command_.responseValue_[17];
  float contactors = command_.responseValue_[2] + command_.responseValue_[6] + command_.responseValue_[10] + command_.responseValue_[14];
  bool brakes = (bool) command_.responseValue_[16];

  complete = driveLidsOpen | (bool) contactors | brakes;
  complete = !complete;


  /* if the last value of the status is not 1, it's not complete */
  bool buzzOff = command_.responseValue_[19] == 0;
  
  complete = complete & buzzOff;

  return complete;
}

unsigned ServoCommsSa::readPositionFault()
{
  return 0;
}


/**............................................................
 *  Record the result from Status to the Register Map
 */
void ServoCommsSa::recordStatusResponse(ServoCommandSa& command)
{
  
  if(command.responseValid_) {
    // store the data
    static float      stat[SERVO_NUMBER_MOTORS];
    static float    cutout[SERVO_NUMBER_MOTORS];
    static float contactor[SERVO_NUMBER_MOTORS];
    static float   circuit[SERVO_NUMBER_MOTORS];
    
    for (unsigned i=0; i < 4; i++){
      stat[i]      =  command.responseValue_[i*4+0];
      cutout[i]    =  command.responseValue_[i*4+1];
      contactor[i] =  command.responseValue_[i*4+2];
      circuit[i]   =  command.responseValue_[i*4+3];
    };
    
    // Write them to shared memory
    
    if(share_) {
      share_->writeReg(servoStatus_,          stat);
      share_->writeReg(servoThermalCutouts_,  cutout);
      share_->writeReg(servoContactors_,      contactor);
      share_->writeReg(servoCircuitBreakers_, circuit);
    
      // now for the last three
      share_->writeReg(servoBrakes_, (bool) command.responseValue_[16]);
      share_->writeReg(driveLids_,   (bool) command.responseValue_[17]);
      share_->writeReg(azWrap_,             command.responseValue_[18]);
    }

  };

  return;
};


/**............................................................
 *  Method that waits
 */
void ServoCommsSa::wait(long nsec) {
  
  struct timespec delay;
  delay.tv_sec = 0;
  delay.tv_nsec = nsec;

  nanosleep(&delay, NULL);
}
 

/**............................................................
 *  Finish the initialization
 */
void ServoCommsSa::finishInitialization()
{

  if(initializationPartOne_ == 0){
    ReportSimpleError("Can not finish initialization.  First part not complete");
    initializationComplete_ = 0;
    return;
  }
  std::vector<float> vals(7);


  COUT("Loading Loop Parameters");
  // 4 different values
  const float pars[7] = {8000, 10, 10, 1, 0.005, 1.5, 1.2 };
  for (unsigned i=0;i<7;i++){
    vals[i] = pars[i];
  };
  issueCommand(ServoCommandSa::LOAD_LOOP_PARAMS_A, vals);
  wait(100000000);

    	vals[0] = 2000.;
    	vals[1] = 2.;
	vals[2] = 0.;
	vals[3] = 1.;
	vals[4] = 0.3;
	vals[5] = 1.;
	vals[6] = 0.7;
  issueCommand(ServoCommandSa::LOAD_LOOP_PARAMS_B, vals);
  wait(100000000);

  	vals[0] = 8000.;
    	vals[1] = 10.;
	vals[2] = 10.;
	vals[3] = 1.;
	vals[4] = 0.05;
	vals[5] = 1.;
	vals[6] = 1.;
  issueCommand(ServoCommandSa::LOAD_LOOP_PARAMS_C, vals);
  wait(100000000);  
	
//	vals[0] = 3000.;
//    	vals[1] = 2.;
//	vals[2] = 0.;
//	vals[3] = 1.;
//	vals[4] = 0.115;
//	vals[5] = 1.;
//	vals[6] = 0.8; 
//reduced the parameters to avoid counterweight issues for now- rebalancing probably means the old ones will work better
	vals[0] = 500.;
    	vals[1] = 0.2;
	vals[2] = 0.;
	vals[3] = 1.;
	vals[4] = 0.13;
	vals[5] = 0.8;
	vals[6] = 0.4; 
 
  issueCommand(ServoCommandSa::LOAD_LOOP_PARAMS_D, vals);
  wait(100000000);
 
  
  




  
  // Lastly we engage the drives
  vals[0] = 1;
  issueCommand(ServoCommandSa::SERVO_ENGAGE, vals);


  initializationComplete_ = 1;

  COUT("Initialization Complete");

};


/**............................................................
 *  Check if circuit breaker is tripped
 */
bool ServoCommsSa::isCircuitBreakerTripped()
{
  static float breakerTrip[4];
  share_->readReg(servoCircuitBreakers_, breakerTrip);

  //  COUT("values [0-3]: " << breakerTrip[0] << "," << breakerTrip[1] << "," << breakerTrip[2] << "," << breakerTrip[3]);
  bool isTripped = false;
  int i;
  for (i=0;i<SERVO_NUMBER_MOTORS;i++){
    isTripped = isTripped | (bool) breakerTrip[i];
  };

  return false;
}


/**............................................................
 *  Check if circuit braked is tripped
 */
bool ServoCommsSa::isThermalTripped()
{
  static float thermalTrip[4];
  share_->readReg(servoThermalCutouts_, thermalTrip);

  bool isTripped = false;
  int i;
  for (i=0;i<SERVO_NUMBER_MOTORS;i++){
    isTripped = isTripped | (bool) thermalTrip[i];
  };

  //  COUT("CHECKING thermal");
  //  COUT("TRIP STATUS: " << isTripped);

  return isTripped;
}


/**............................................................
 *  Check if circuit braked is tripped
 */
bool ServoCommsSa::isLidOpen()
{
  bool lidStatus;
  share_->readReg(driveLids_, &lidStatus);

  return lidStatus;
}


/**............................................................
 *  Check if circuit braked is tripped
 */
bool ServoCommsSa::isBrakeOn()
{
  bool brakeStatus;
  share_->readReg(servoBrakes_, &brakeStatus);

  return brakeStatus;
}


/**............................................................
 *  Check if everything's initialized
 */
bool ServoCommsSa::isInitialized()
{
  return initializationComplete_;
}
