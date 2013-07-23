/**
 *  CbassPowerSupply.cc
 *  class to talk to the cbass power supply
 *  sjcm
 */

#include "gcp/util/specific/CbassPowerSupply.h"
#include <string.h>
#include <sys/ioctl.h>
#include "gcp/util/common/usb.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/String.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/control/code/unix/libunix_src/common/tcpip.h"
#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <fcntl.h>


#define DEFAULT_PS_PORT "/dev/ttyLnapsu"

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructors
 */
// no shared resources
CbassPowerSupply::CbassPowerSupply() 
{
  // set up some of the variables

  connected_  = false;
}


/**.......................................................................
 * Destructors
 */
CbassPowerSupply::~CbassPowerSupply() {

  // must disconnected
  psDisconnect();
    
}


/**.......................................................................
 * Connect
 */
void CbassPowerSupply::psConnect() 
{

  struct termios toptions;
  
  // Open the serial connection.  Throw up an error if failed.
  fd_ = open(DEFAULT_PS_PORT,O_RDWR|O_NOCTTY|O_NDELAY);
  COUT("FD_ " << fd_);
  if (fd_ == -1) {
    ThrowError("psConnect: Unable to open port ");
    connected_ = false;
    return;
  }

  COUT("Getting baud rates");
  // Get the attributes.
  if (tcgetattr(fd_, &toptions) < 0) {
    ThrowError("psConnect: Could not get term attributes");
    connected_ = false;
    return;
  }

  COUT("Setting baud rates");
  // Set the IO baud rate.
  cfsetispeed(&toptions, BRATE);
  cfsetospeed(&toptions, BRATE);

  // 8N1
  toptions.c_cflag &= ~PARENB;
  toptions.c_cflag &= ~CSTOPB;
  toptions.c_cflag &= ~CSIZE;
  toptions.c_cflag |= CS8;
  // no flow control
  toptions.c_cflag &= ~CRTSCTS;

  toptions.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
  toptions.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

  toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
  toptions.c_oflag &= ~OPOST; // make raw

  // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
  toptions.c_cc[VMIN]  = 0;
  toptions.c_cc[VTIME] = 20;

  // Set the attributes.
  if( tcsetattr(fd_, TCSANOW, &toptions) < 0) {
    ThrowError("psConnected: Could not set attributes");
    connected_ = false;
    return;
  }

  // wait two seconds
  struct timespec delay;
  delay.tv_sec  =         3;
  delay.tv_nsec =         0;

  nanosleep(&delay, 0);

  connected_ = true;
  fdSet_.registerReadFd(fd_);
  fdSet_.registerExceptionFd(fd_);

  queryBias_ = true;

  return;
}


/**.......................................................................
 * Disconnect
 */
void CbassPowerSupply::psDisconnect()
{

  // don't do anything if we're not connected
  if(!connected_){
    COUT("not connected");
    return;
  }

  // if connected, try to exit cleanly
  char modStage[2];
  modStage[0] = 0x03;
  modStage[1] = 0x00;

  psSetModule(modStage);
  if(retVal_ < 0) {
    ThrowError("psDisconnect: module not safely set");
    return;
  }

  close(fd_);

  fd_ = -1;
  connected_ = false;

  return;
};

/**.......................................................................
 * Low-level write command
 */
int CbassPowerSupply::psWrite(char* command)
{
  if(!connected_){
    COUT("not connected to power supply");
    return -1;
  }

  int retVal = write(fd_, command, strlen(command));

  return retVal;
}

/**.......................................................................
 * Low-level write command
 */
int CbassPowerSupply::psWrite(char* command, int len)
{
  if(!connected_){
    COUT("not connected to power supply");
    return -1;
  }
  
  //  int i;
  //  for (i=0;i<len;i++){
  //    printBits(*(command + i));
  //  }
  int retVal = write(fd_, command, len);

  return retVal;
}

#if(0)
/**.......................................................................
 * Low-level read command
 */
int CbassPowerSupply::psRead()
{
  if(!connected_){
    COUT("not connected to power supply");
    return -1;
  }

  int i, j, n, selReturn;
  char b[1];
  b[0] = 0x00;

  i = 0; 
  j = 0;

  // The read loop.  Maximum buffer length is max_read_buf.
  do {
    // Check length of returned buffer and timeout counter.
    if ( j>PS_READ_ATTEMPTS_TIMEOUT ) {
      ThrowError("psRead: Timeout on read.");
      return -1;
    }
    else if ( i==PS_MAX_READ_BUF ) {
      COUT("psRead: Received buffer exceeds PS_MAX_READ_BUF.");
      return -1;
    }
    
    COUT("FD_: " << fd_);
    n = read(fd_, b, 1);  // read a char at a time
    COUT("B[0]: " << b[0]);
    COUT("n: " << n);
    if(n<0){
      COUT("psRead: could not read character");
      return -1;
    }
    if( n==0 ) {
      selReturn = waitForResponse();
      if(selReturn ==0){
	j++;
      }
      continue;
    }
    returnString_[i] = b[0]; 
    i++; 
  } while( b[0] < PS_BUF_LIMIT );

  retVal_ = atoi(returnString_);

  return (int) retVal_;
}


int CbassPowerSupply::psRead2()
{
  if(!connected_){
    COUT("not connected to power supply");
    return -1;
  }

  int i, j, n, selReturn, nbyte;
  int numTimeout= 0;
  char b[1];
  b[0] = 0x00;

  i = 0; 
  j = 0;
  int stopLoop= 0;


  do{
    try {
      selReturn = waitForResponse();
    } catch(...){
      if(selReturn ==0){
	numTimeout++;
      } 
    }
    
    if(selReturn>0){
      n = read(fd_, b, 1);  // read a char at a time
      if(n<0){
	ThrowError("psRead: could not read character that should be present");
	return -1;
      }
      returnString_[i] = b[0]; 
      i++;
    }
    

    // check if the message is over
    if(b[0] == PS_BUF_LIMIT ){
      stopLoop = 1;
      COUT("RETURNSTRING: " << returnString_);
      retVal_ = atoi(returnString_);
      COUT("retVal_: " << retVal_);
      return (int) retVal_;
    };

    // check the limits
    if(numTimeout > PS_READ_ATTEMPTS_TIMEOUT){
      COUT("TIMEOUT ON READ");
      ThrowError("psRead: Timeout on read.");
      stopLoop = 1;
      return -1;
    }

    if( i==PS_MAX_READ_BUF) { 
      COUT("MAX BUFFER EXCEEDED");
      ThrowError("psRead: Received buffer exceeds PS_MAX_READ_BUF.");
      stopLoop = 1;
    }
  }while(stopLoop==0); 

}

#endif

// this one assumes commas separating things, and transmissions end with 'x';
int CbassPowerSupply::readArduino()
{
  if(!connected_ || fd_ <0){
    COUT("not connected to power supply");
    return -1;
  }


  int i,nbyte,waserr=0,nread=0;
  unsigned char line[PS_MAX_READ_BUF], *lptr=NULL;
  int ioctl_state=0;
  bool stopLoop = 0;
  TimeVal start, stop, diff;

  // Start checking how long this is taking.                                                   

  start.setToCurrentTime();

  // Set the line pointer pointing to the head of the line                                     

  lptr = line;

  do {

    //    COUT("About to wait for response");                                                  
    waitForResponse();
    //    COUT("About to wait for response... done");                                          

    ioctl_state=ioctl(fd_, FIONREAD, &nbyte);
    //    COUT("Found " << nbyte << " bytes");
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
     * Read the bytes one at a time                                                            
     */
    for(i=0;i < nbyte;i++) {
      if(read(fd_, lptr, 1) < 0) {
        // THROW ERRORS                                                                        
        COUT("readArduino: Read error.\n");
        return 0;
      };

      if(*lptr !=  PS_BUF_LIMIT && nread < PS_MAX_READ_BUF-1) {
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
    if(diff.getTimeInMicroSeconds() > PS_TIMEOUT_USEC){
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
  strcpy(returnString_, (char*) line);

  //  CTOUT("Exiting loop: line = " << line);                                                  
  return 1;
}



/**.......................................................................
 * Mid-level set module command
 */
void CbassPowerSupply::psSetModule(char* modStage){
  
  if(!connected_){
    COUT("not connected to power supply");
    return;
  }

  char command[3];
  int retVal;

  // Setup the command string.
  command[0] = 0x00;
  command[1] = modStage[0];
  command[2] = modStage[1];
  
  // Send the command string to the arduino.
  retVal = psWrite(command, 3);
  
  if( retVal<0 ) {
    ThrowError("psSetModule: psWrite failed.");
    return;
  }


  // read the response
  retVal = readArduino();

  // check the response
  if(retVal != 1){
    ThrowError("failed to read arduino");
    return;
  }

  // parse the response
  std::string respStr(returnString_);
  String responseString(respStr);
  float retVal1 = responseString.findNextStringSeparatedByChars("y").toFloat();
  switch( (int) retVal1){
  case -1:
    ThrowError("psSetModule: readArduino failed.");
    break;

  case -11:
    ThrowError("psSetModule: module not changed, wrong number of arguments.");
    break;

  case -12:
    ThrowError("psSetModule: module not changed, out of range.");
    break;

  case -13:
    ThrowError("psSetmodule: module not changed, module & stage not selected.");
    break;

  default:
    break;
  }
  
  // Success!
  return;
}



/**.......................................................................
 * Mid-level change voltage command
 */
void CbassPowerSupply::psChangeVoltage(char* voltRequest)
{
  if(!connected_){
    COUT("not connected to power supply");
    return;
  }

  char command[4];
  int retVal;

  // Setup the command string.
  command[0] = 0x01;
  command[1] = voltRequest[0];
  command[2] = voltRequest[1];
  command[3] = voltRequest[2];

  COUT("VOLTREQUEST: " << (int) voltRequest[0] << "," << (int) voltRequest[1] << "," << (int) voltRequest[2]);

  // Send the command string to the arduino.
  retVal = psWrite(command,4);
  // Check to ensure it happened correctly.
  if( retVal==-1 ) {
    ThrowError("psChangeVoltage: psWrite failed.");
    return;
  }


  //  COUT("READING RESPONSE");
  // read the response
  retVal = readArduino();

  // check the response
  if(retVal != 1){
    ThrowError("failed to read arduino");
    return;
  }

  // parse the response
  std::string respStr(returnString_);
  String responseString(respStr);
  float retVal1 = responseString.findNextStringSeparatedByChars("y").toFloat();
  switch( (int) retVal1){
  case -1:
    ThrowError("psChangeVoltage: readArduino failed.");
    break;

  case -11:
    ThrowError("psChangeVoltage: voltage not changed, wrong number of arguments.");
    break;

  case -12:
    ThrowError("Voltage not changed, Values out of range.");
    break;

  case -13:
    ThrowError("psChangeVoltage: voltage not changed, module & stage not selected.");
    break;

  default:
    break;
  }

  //  COUT("DONE SETTING VOLTAGE");
  // Success!
  return;
}
  

#if(0)
/**.......................................................................
 * Mid-level get voltage command
 */
void CbassPowerSupply::psGetVoltageOld()
{
  if(!connected_){
    COUT("not connected to power supply");
    return;
  }

  char command[1];
  int retVal;
  int i;

  // Setup the command string.
  command[0] = 0x02;
  
  // Send the command string to the arduino.
  retVal = psWrite(command);
  // Check to ensure it happened correctly.
  if( retVal==-1 ) {
    ThrowError("psGetVoltage: psWrite failed.");
    return;
  }

  for ( i=0; i<3; i++) {
    // Wait for the return value.
    retVal = psRead2();
    // Check return value.
    switch (retVal) {
    case -1:
      ThrowError("psGetVoltage: psRead failed.");
      return;
      break;
    case -13:
      ThrowError("psGetVoltage: voltage not read, module & stage not selected.");
      return;
      break;
    default:
      break;
    }

    // Set the returned voltage to its proper place.
    volts_[i] = (float) retVal;
  }
  
  // Success!
  return;
}
#endif

/**.......................................................................
 * Mid-level get voltage command
 */
void CbassPowerSupply::psGetVoltage()
{
  if(!connected_){
    COUT("not connected to power supply");
    return;
  }

  char command[1];
  int retVal;
  int i;

  // Setup the command string.
  command[0] = 0x02;
  
  // Send the command string to the arduino.
  retVal = psWrite(command);
  // Check to ensure it happened correctly.
  if( retVal==-1 ) {
    ThrowError("psGetVoltage: psWrite failed.");
    return;
  }

  // read the response
  retVal = readArduino();

  // check the response
  if(retVal != 1){
    ThrowError("failed to read arduino");
    return;
  }

  // parse the response
  std::string respStr(returnString_);
  String responseString(respStr);
  float retVal1 = responseString.findNextStringSeparatedByChars("y").toFloat();
  switch( (int) retVal1){
  case -1:
    ThrowError("psGetVoltage: psRead failed.");
    break;

  case -13:
    ThrowError("psGetVoltage: voltageNot read, module & stage not selected");
    break;

  default:
    volts_[0] = retVal1;
    for (i=1;i<3;i++){
      volts_[i] = responseString.findNextStringSeparatedByChars("y").toFloat();
    };
    break;
  }

  // Success!
  return;
}


/**.......................................................................
 * High-level set voltage command
 */
int CbassPowerSupply::psSetVoltage(int modsel, int stagesel, float drainvolt, float draincurrent)
{
  
  if(!connected_){
    COUT("not connected to power supply");
    return -1;
  }
  
  struct timespec delay;
  delay.tv_sec  =         0;
  delay.tv_nsec = 250000000;

  char modstage[2];
  char voltreq[3];
  int volts[3];
  int i;
  int j;
  int r;

  float min_draincurrent;
  float max_draincurrent;

  float drainVoltDU;
  float drainCurrentDU;

  float drainVoltDiff;
  float drainCurrentDiff;
  float drainAllow;
  float currentAllow;

  // Start off by checking the requested values.  If they are in the acceptable range,
  // then we may continue.
  
  if (drainvolt < 0 || drainvolt > max_drainvolt[stagesel-1]) {
    ThrowError("ps_setvoltage: requested drain voltage out of range");
    return -1;
  }
  
  min_draincurrent = mindraincurrent_slope[stagesel-1] *
    drainvolt + mindraincurrent_intercept[stagesel-1];
  max_draincurrent = maxdraincurrent_slope[stagesel-1] *
    drainvolt + maxdraincurrent_intercept[stagesel-1];
  
  if (draincurrent < min_draincurrent || draincurrent > max_draincurrent) {
    ThrowError("ps_setvoltage: requested drain current out of range");
    return -1;
  }
  

  // The algorithm will be to check the voltages, estimate the number of steps for
  // the gate voltage.  Then change the gate voltage.  Check, and try to zero in on the
  // desired value.  Then do something similar for the drain voltage, except that we are
  // trying to match the desired current.  This is a bit less easy and needs some
  // experimentation, I think.
  
  // First: select the module and stage.
  
  // Set up the command.
  modstage[0] = (char) modsel;
  modstage[1] = (char) stagesel;
  
  // Send it.
  try{
    psSetModule(modstage);
  } catch(...) {
    COUT("psSetVoltage: module and stage selection failed.");
    return -1;
  }

  // Take stock of our current situation:
  try{
    psGetVoltage();
  } catch (...) {
    COUT("psSetVoltage: could not read current voltages.");
    return -1;
  }

  // OK, now want to convert the desired drain voltage and current to DU:
  drainVoltDU = (drainvolt - PS_VOLT_B_DRAIN) / PS_VOLT_M_DRAIN;
  drainCurrentDU = (draincurrent - PS_GAIN_B_CURRENT) / PS_GAIN_M_CURRENT;

  // Take stock of our current situation:
  try{
    psGetVoltage();
  } catch (...) {
    COUT("psSetVoltage: could not read current voltages.");
    return -1;
  }

  // How far are we currently from the desired values?
  drainVoltDiff = drainVoltDU - volts_[1];
  drainCurrentDiff = drainCurrentDU - volts_[2];


  // Calculate our actual allowance:
  drainAllow = drain_stepsize[stagesel-1] / 2;
  currentAllow = current_stepsize[stagesel-1] / 2;

  i = 0;

  // Enter the main loop:
  while (fabs(drainVoltDiff) > drainAllow || fabs(drainCurrentDiff) > currentAllow) {
    //    COUT("IN MAIN LOOP");
    // Drain voltage loop:
    i = 0;
    while (fabs(drainVoltDiff) > drainAllow && i < PS_SETVOLT_TIMEOUT) {
      // Check that we haven't taken too long, here.
      if( i==PS_SETVOLT_TIMEOUT ) {
	ThrowError("ps_setvoltage: gate voltage has not converged.");
	break;
      }
      i++;

      // Set up the change voltage command.
      voltreq[0] = (char) (drainVoltDiff > 0);        // Increment/Decrement.
      voltreq[1] = 0x01;                                              // Drain Voltage
      voltreq[2] = (char) round(fabs(drainVoltDiff) / drain_stepsize[stagesel-1]);

      // Change the voltages.
      try {
        psChangeVoltage(voltreq);
      } catch (Exception& err) {
	//	COUT("psSetVoltage: could not set voltages.");
	ReportSimpleError("FAIL. " << err.what());
	//	COUT("ERROR: " << err.what());
	return -1;
      }
      nanosleep(&delay, 0);

      // Get the current voltages.
      try {
	psGetVoltage();
      } catch(...) {
	COUT("psSetVoltage: could not read current voltages.");
	return -1;
      }
      // Get the remaining difference.
      drainVoltDU = (drainvolt - PS_VOLT_B_DRAIN) / PS_VOLT_M_DRAIN;
      drainVoltDiff = drainVoltDU - volts_[1];

    };
    

      // Check that we didn't taken too long, there.
      if( i==PS_SETVOLT_TIMEOUT ) {
	ThrowError("ps_setvoltage: drain voltage has not converged");
	break;
      }
    

    // Calculate this guy again.
    drainCurrentDU = (draincurrent - PS_GAIN_B_CURRENT) / PS_GAIN_M_CURRENT;
    drainCurrentDiff = drainCurrentDU - volts_[2];
    
    // Drain current loop:
    i = 0;
    while(fabs(drainCurrentDiff) > currentAllow && i < PS_SETVOLT_TIMEOUT) {
      
      i++;

      // If the lower limit is requested, send it straight there.
      if (draincurrent == 0) {
	
	// Set up the change voltage command.
	voltreq[0] = 0x01;      // Increment/Decrement.
	voltreq[1] = 0x00;      // Gate
	voltreq[2] = 0xd0;
	
	// Change the voltages.
	try {
	  psChangeVoltage(voltreq);
	} catch (...) {
	  COUT("psSetVoltage: could not set voltages.");
	  return -1;
	}
	nanosleep(&delay, 0);
	

	// If we're out of the linear regime and it's the first iteration, then rail it and
	// bring it back.
      } else if (i == 0 && (volts_[2] < nonlin_current || volts_[0] < nonlin_gate)) {
	
	// Set up the change voltage command.
	voltreq[0] = 0x01;      // Increment/Decrement.
	voltreq[1] = 0x00;      // Gate
	voltreq[2] = 0xd0;
	// Change the voltages.
	try {
	  psChangeVoltage(voltreq);
	} catch (...) {
	  COUT("psSetVoltage: could not set voltages.");
	  return -1;
	}
	nanosleep(&delay, 0);


        // Set up the change voltage command.
	voltreq[0] = 0x00;      // Increment/Decrement.
	voltreq[1] = 0x00;      // Gate
	voltreq[2] = (char) round(drainCurrentDU / current_stepsize[stagesel-1]);

	// Change the voltages.
	try {
	  psChangeVoltage(voltreq);
	} catch (...) {
	  COUT("psSetVoltage: could not set voltages.");
	  return -1;
	}
	nanosleep(&delay, 0);


      } else {
	// Set up the change voltage command.
	voltreq[0] = (char) (drainCurrentDiff < 0);     // Increment/Decrement.
	voltreq[1] = 0x00;                                                      // Gate
	voltreq[2] = (char) round(fabs(drainCurrentDiff) / current_stepsize[stagesel-1]);

	// Change the voltages.
	try {
	  psChangeVoltage(voltreq);
	} catch (...) {
	  COUT("psSetVoltage: could not set voltages.");
	  return -1;
	}
	nanosleep(&delay, 0);
  
	
      }

      // Get the current voltages.
      try{
	psGetVoltage();
      } catch (...) {
	COUT("psSetVoltage: could not read current voltages.");
	return -1;
      }
      
      // Get the remaining difference.
      drainCurrentDU = (draincurrent - PS_GAIN_B_CURRENT) / PS_GAIN_M_CURRENT;
      drainCurrentDiff = drainCurrentDU - volts_[2];
    }; 

    if( i==PS_SETVOLT_TIMEOUT ) {
      ThrowError("ps_setvoltage: drain current has not converged");
      break;
    }


    // Calculate this guy again.
    drainVoltDU = (drainvolt - PS_VOLT_B_DRAIN) / PS_VOLT_M_DRAIN;
    drainVoltDiff = drainVoltDU - volts_[1];
  }; 

  //  COUT("OUT OF MAIN LOOP");

  // Finally: deselect the stage in order to store the desired values.
  
  // Set up the command.
  modstage[0] = (char) 0x03;
  modstage[1] = (char) 0x00;
  
  // Send it.
  try{
    psSetModule(modstage);
  } catch (...) {
    COUT("psSetVoltage: module and stage deselection failed.");
    return -1;
  }
  
  //  COUT("SUCCESSFULLY CHANGED VOLTAGE");

  // Success!
  return 0;
}


/**.......................................................................
 * High-level get all voltages command
 */
int CbassPowerSupply::psGetAllVoltages()
{
  
  if(!connected_){
    ThrowError("not connected to power supply");
    return -1;
  }

  struct timespec delay;
  delay.tv_sec  =         0;
  delay.tv_nsec =  1000000;

  char modstage[2];
  int i;
  int j;
  int retVal;


  // We'll just switch through the modules and stages.  Just a big loop, basically.
  for ( i=0; i<2; i++) {
    for ( j=0; j<6; j++) {

      // First: select the module and stage.

      // Set up the command.
      modstage[0] = (char) i+1;
      modstage[1] = (char) j+1;
      
      // Send it.
      //      COUT("[I, J]: [" << i << "," << j << "]"); 
      try{
	psSetModule(modstage);
	//	nanosleep(&delay, 0);
      } catch(...) {
	COUT("psGetAllVoltages: module and stage selection failed.");
	return -1;
      }

      // Get the current voltages.
      try{
	psGetVoltage();
      } catch(...) {
	COUT("psGetAllVoltages: could not read current voltages.");
	return -1;
      }

      allVolts_[18*i + 3*j]     = volts_[0] * PS_VOLT_M_GATE + PS_VOLT_B_GATE;
      allVolts_[18*i + 3*j + 1] = volts_[1] * PS_VOLT_M_DRAIN + PS_VOLT_B_DRAIN;
      allVolts_[18*i + 3*j + 2] = volts_[2] * PS_GAIN_M_CURRENT + PS_GAIN_B_CURRENT;
      
      //COUT("voltmgate, voltbgate: " << PS_VOLT_M_GATE << "," << PS_VOLT_B_GATE);
      //COUT("VOLTS_[0], ALLVOLTS_[O]: " << volts_[0] << "," <<allVolts_[18*i + 3*j]);
      //COUT("VOLTS_[1], ALLVOLTS_[1]: " << volts_[1] << "," <<allVolts_[18*i + 3*j + 1]);
      //COUT("VOLTS_[2], ALLVOLTS_[2]: " << volts_[2] << "," <<allVolts_[18*i + 3*j + 2]);
      
    }
  }

  // Success!
  return 0;
}

/*.......................................................................
 * Print the bits of a 1-byte unsigned char.
 */
void CbassPowerSupply::printBits(unsigned char feature)
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



/**.......................................................................
 * Wait for a response from the arduino card
 */
int CbassPowerSupply::waitForResponse()
{
  TimeVal timeout(0, PS_TIMEOUT_USEC, 0);

  // Do nothing if we are not connected to the arduino
  if(!connected_)
    return -1;

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
  return nready;
}

