/**
 *  CbassBackend.cc 
 *  class to talk to the cbass backend.
 *  sjcm
 */

#include "gcp/util/specific/CbassBackend.h"
#include <string.h>
#include <sys/ioctl.h>
#include "gcp/util/common/usb.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/control/code/unix/libunix_src/common/tcpip.h"
#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"
#include <iostream>
#include <fstream>
#include <cstdlib>

//#define DEF_HEX_FILE "/home/sjcm/code/cbass/back/cbass_1209.hex"
//#define DEF_HEX_FILE "/home/sjcm/code/cbass/back/cbass_20101119.hex"
//#define DEF_HEX_FILE "/home/sjcm/code/cbass/back/cbass_20111012.hex"
#define DEF_HEX_FILE "/home/cbass/gcpCbass/util/specific/cbass.hex"


using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructors
 */
// no shared resources
CbassBackend::CbassBackend(): 
  sortTimeVals_(RECEIVER_SAMPLES_PER_FRAME), sortTimeVals2_(RECEIVER_SAMPLES_PER_FRAME), sortDataVals_(RECEIVER_SAMPLES_PER_FRAME, std::vector<float>(NUM_RECEIVER_SWITCH_CHANNELS)), sortDiagnostics_(RECEIVER_SAMPLES_PER_FRAME, std::vector<float>(NUM_RECEIVER_DIAGNOSTICS)), sortRegData_(RECEIVER_SAMPLES_PER_FRAME, std::vector<float>(NUM_RECEIVER_CHANNELS)), sortAlpha_(RECEIVER_SAMPLES_PER_FRAME, std::vector<float>(NUM_ALPHA_CORRECTIONS)), sortNonlin_(RECEIVER_SAMPLES_PER_FRAME, std::vector<float>(NUM_RECEIVER_SWITCH_CHANNELS)), sortFlags_(RECEIVER_SAMPLES_PER_FRAME), sortBackendVersion_(RECEIVER_SAMPLES_PER_FRAME), sortAvgSec_(RECEIVER_SAMPLES_PER_FRAME), timeVals_(RECEIVER_SAMPLES_PER_TRANSFER), timeVals2_(RECEIVER_SAMPLES_PER_TRANSFER), dataVals_(RECEIVER_SAMPLES_PER_TRANSFER, std::vector<float>(NUM_RECEIVER_SWITCH_CHANNELS)), flags_(RECEIVER_SAMPLES_PER_TRANSFER), packetVals_(RECEIVER_SAMPLES_PER_TRANSFER)
{
  // set up some of the variables

  connected_  = false;
  startIndex_ = 0;

#if DIR_HAVE_USB
  devHandle_  = NULL;
#endif
}


/**.......................................................................
 * Destructors
 */
CbassBackend::~CbassBackend() {

  // must disconnected
  backendDisconnect();
    
}


/**.......................................................................
 * Connect
 */
void CbassBackend::backendConnect() 
{
#if DIR_HAVE_USB
  usb_dev_handle *temp_handle;
  int ret=0;
  int count;
  int i = 0;
  int interfaceNumber;
  int alternateSetting;
  bool foundMatch = false;
  
  COUT("Initializing USB port");
  usb_init();
  COUT("Setting debug level");
  usb_set_debug(3);
  COUT("Finding USB busses");
  usb_find_busses();
  COUT("Finding Devices");
  usb_find_devices();
  
  struct usb_bus* busses = usb_get_busses();
  struct usb_bus* bus=0;

  // now we loop through the usb devices and find the one that matches.
  for (bus = busses; bus; bus = bus->next) {
    struct usb_device *dev;
    for (dev = bus->devices; dev; dev = dev->next) {
      struct usb_device_descriptor desc = dev->descriptor;
      COUT("vendorID is: " << desc.idVendor);
      COUT("prodID is: " << desc.idProduct);

      if ((desc.idVendor == VendorID) & (desc.idProduct == ProductID)) {	

	COUT("Found a Match to Backend");
	temp_handle = usb_open(dev);
	interfaceNumber = dev->config->interface->altsetting->bInterfaceNumber;
	alternateSetting = dev->config->interface->altsetting->bAlternateSetting;
	foundMatch = true;
	break;
      }
    }
  }
  // if no matches were found, throw an error
  if(!foundMatch){
    COUT("no match found");
    ThrowError("No Match to Backend Found");
  }

  // claiming the interface
  ret = usb_claim_interface(temp_handle, interfaceNumber);
  if (ret < 0) {
    COUT("Error claiming USB interface");
    ThrowError("Error claiming USB interface");
    connected_ = false;
    return;
  }

  // setting alternate interface
  ret = usb_set_altinterface(temp_handle, 1);
  if (ret < 0) {
    COUT("Error setting alternate interface");
    ThrowError("Error setting alternate interface");
    connected_ = false;
    return;	
  }

  // otherwise we're connected, and we set the variables in the class
  devHandle_ = temp_handle;
  connected_ = true;

#endif
  return;
}

/**.......................................................................
 * Disconnect
 */
void CbassBackend::backendDisconnect()
{
#if DIR_HAVE_USB
  // don't do anything if we're not connected
  if(!connected_){
    COUT("not connected");
    return;
  }
  
  // if we are connected, do a bunch of stuff
  int ret;
  int currentInterface;
  
  struct usb_device* device;
  device = usb_device(devHandle_);
  currentInterface = device->config->interface->altsetting->bInterfaceNumber;
  
  ret = usb_release_interface(devHandle_, currentInterface);
  if (ret < 0) {
    ThrowError("Could not release interface");
    connected_ = true;
    return;
  }
  
  // closing device
  usb_close(devHandle_);
  
  // all good
  connected_ = false;
  devHandle_ = NULL;

#endif
  return;
}

/**.......................................................................
 * Read the hex file
 */
void CbassBackend::hexRead(FILE* hexFile) 
{
#if DIR_HAVE_USB
  // some variables.
  char c;
  uint16_t i;
  int n, c1, check, len;
  c = getc(hexFile);
  
  if(c != ':'){
    ThrowError("Start of hex file not as expected");
    return;
  }
  
  n = fscanf(hexFile, "%2lX%4lX%2lX", &record_.Length, &record_.Address, &record_.Type);
  if(n != 3){
    ThrowError("hex file not of correct type");
    return;
  }
  
  len = record_.Length;
  if(len > MAX_HEX_LENGTH){
    ThrowError("hex too long");
    return;
  }
  for(i = 0; i<len; i++){	
    n = fscanf(hexFile, "%2X", &c1);
    if(n != 1){
      if(i != record_.Length){
	ThrowError("hex not of correct length");
	return;	
      }
    }
    record_.Data[i] = c1;
    
  }
  n = fscanf(hexFile, "%2X\n", &check);
  if(n != 1){
    ThrowError("could not scan file properly");
    return;
  }
#endif
  return;
}

/**.......................................................................
 * Load the hex file
 */
void CbassBackend::loadHex()
{
#if HAVE_USB
  int ret, ret_hex;
  uint8_t RequestType = 0x40;
  uint8_t Request = 0xA0;
  char reset;
  FILE *hexFile;
  int transfer_count = 0;
  
  hexFile = fopen(DEF_HEX_FILE, "r");
  if(hexFile == NULL){
    ThrowError("Could not Open Hex file");
    return;
  }
  
  // Assert reset
  reset = 0x01;
  ret = usb_control_msg(devHandle_, RequestType, Request, cy8051_CPUCS, 0, &reset, 1, BACKEND_TIMEOUT);
  if (ret < 0) {
    ThrowError("USB_control_msg fail");
    return;
  }  
  
  while(1){
    try{
      hexRead(hexFile);
    } catch (...) {
      ThrowError("Hex File could not be read");
      return;
    }

    if(record_.Type != 0)
      break;

    ret = usb_control_msg(devHandle_, RequestType, Request, record_.Address, 
			  0, record_.Data, record_.Length, BACKEND_TIMEOUT);
    if (ret < 0){
      ThrowError("Could not download file");
      return;
    }
    transfer_count += record_.Length;
  }

  // De-assert reset
  reset = 0x00;
  ret = usb_control_msg(devHandle_,RequestType,Request,cy8051_CPUCS,0,&reset,1, BACKEND_TIMEOUT);
  if (ret < 0) {
    ThrowError("Could not reset USB port");
    return;
  }
#endif  
  return;
}

/**.......................................................................
 * Generalized issue command
 */
int CbassBackend::issueCommand(Command type)
{
  // just define all the other values and move on
  unsigned char address;
  unsigned char* period;
  return issueCommand(type, address, period);
}
int CbassBackend::issueCommand(Command type, unsigned char address)
{
  // just define all the other values and move on
  unsigned char* period;
  return issueCommand(type, address, period);
}
int CbassBackend::issueCommand(Command type, unsigned char* period)
{
  // just define all the other values and move on
  unsigned char address;
  return issueCommand(type, address, period);
}
int CbassBackend::issueCommand(Command type, unsigned char address, unsigned char* period)
{
  // just deifne the other values and move on
  unsigned char channel;
  unsigned char stage;
  return issueCommand(type, address, period, channel, stage);
}
int CbassBackend::issueCommand(Command type, unsigned char address, unsigned char* period, unsigned char channel, unsigned char stage)
{
  // now we define the real deal
  
  char data[10];
  int retVal;
  struct timespec delay;
  delay.tv_sec  =         0;
  delay.tv_nsec =   1000000;  // 1 ms

  switch(type) {
  case READ_DATA:
    // this is the only case where we don't do a write.  just issue it and return from the function
    retVal = getData();
    return retVal;
    break;
    
  case FPGA_RESET:
    data[0] = 0xAA;
    data[1] = 0xFF;
    data[2] = '\0';
    break;
    
  case FIFO_RESET:
    data[0] = 0xAA;
    data[1] = 0xFD;
    data[2] = '\0';
    break;

  case SET_SWITCH_PERIOD:
    data[0] = 0xAA;
    data[1] = 0xB0;
    data[2] = address;
    data[3] = '\0';
    break;

    /*
  case SET_INT_PERIOD:
    data[0] = 0xAA;
    data[1] = 0xB1;
    data[2] = period[3];  // 4byte number [3] is most sig. default is [1] to c8
    data[3] = period[2];
    data[4] = period[1];
    data[5] = period[0];
    data[6] = '\0';
    break;
    */
    
  case SET_BURST_LENGTH:
    data[0] = 0xAA;
    data[1] = 0xB2;
    data[2] = period[2];
    data[3] = period[1];
    data[4] = period[0];
    data[5] = '\0';
    break;
    
  case SETUP_ADC:
    data[0] = 0xAA;
    data[1] = 0x80;
    data[2] = '\0';
    break;
    
  case TRIGGER:
    data[0] = 0xAA;
    data[1] = 0x40;
    data[2] = '\0';
    break;
    
  case ACQUIRE_DATA:
    data[0] = 0xAA;
    data[1] = 0x41;
    data[2] = address;
    data[3] = '\0';
    break;
    
  case ENABLE_CONTINUOUS:
    data[0] = 0xAA;
    data[1] = 0x01;
    data[2] = address;
    data[3] = '\0';
    break;
    
  case ENABLE_SIMULATOR:
    data[0] = 0xAA;
    data[1] = 0x03;
    data[2] = address;
    data[3] = '\0';
    break;
    
  case ENABLE_NOISE:
    data[0] = 0xAA;
    data[1] = 0x05;
    data[2] = address;
    data[3] = '\0';
    break;
    
  case ENABLE_WALSH:
    data[0] = 0xAA;
    data[1] = 0x07;
    data[2] = address;
    data[3] = '\0';
    break;
    
  case ENABLE_WALSH_ALT:
    data[0] = 0xAA;
    data[1] = 0x09;
    data[2] = address;
    data[3] = '\0';
    break;

  case ENABLE_WALSH_FULL:
    data[0] = 0xAA;
    data[1] = 0x0A;
    data[2] = address;
    data[3] = '\0';
    break;

  case NON_LINEARITY:
    data[0] = 0xAA;
    data[1] = 0x0B;
    data[2] = address;
    data[3] = '\0';
    break;

  case WALSH_TRIM_LENGTH:
    data[0] = 0xAA;
    data[1] = 0xB3;
    data[2] = address;
    data[3] = '\0';
    break;

  case SET_NONLIN:
    COUT("about to set_nonlin");
    data[0] = 0xAA;
    data[1] = 0xB4;
    data[2] = channel;
    data[3] = stage;
    data[4] = period[2];
    data[5] = period[1];
    data[6] = period[0];
    data[7] = '\0';
    break;

  case SET_ALPHA:
    COUT("about to set_alpha");
    data[0] = 0xAA;
    data[1] = 0xB5;
    data[2] = channel;
    data[3] = stage;
    data[4] = period[2];
    data[5] = period[1];
    data[6] = period[0];
    data[7] = '\0';
    break;


  case ENABLE_ALPHA:
    COUT("about to enable_alpha");
    data[0] = 0xAA;
    data[1] = 0x0C;
    data[2] = address;
    data[7] = '\0';
    break;

  }

  // issue the command
  retVal = bulkTransfer(data, sizeof(data));
  
  // do a minor sleep to ensure we're not overloading the fpga
  nanosleep(&delay,0);

  return retVal;
}


/**.......................................................................
 * Reset the FPGA
 */
int CbassBackend::fpgaReset()
{
  char data[2]; 
  data[0] = 0xAA;
  data[1] = 0xFF;
  
  return bulkTransfer(data);
}

/**.......................................................................
 * USB reset
 */
int CbassBackend::usbReset()
{
  char data[2]; 
  data[0] = 0xAA;
  data[1] = 0xFE;

  return bulkTransfer(data);
}

/**.......................................................................
 * FIFO reset
 */
int CbassBackend::fifoReset()
{
  char data[2]; 
  data[0] = 0xAA;
  data[1] = 0xFD;
  
  return bulkTransfer(data);
}

/**.......................................................................
 * Set SW period
 */
int CbassBackend::setSwPeriod(unsigned char period)
{
  char data[3]; 
  data[0] = 0xAA;
  data[1] = 0xB0;
  data[2] = period;
  
  return bulkTransfer(data);
}

/**.......................................................................
 * Set Integration period
 */
int CbassBackend::setIntPeriod(unsigned char* period)
{
  /*
  unsigned char period[4];
  period[0] = 0;
    period[1] = 0xc8;  // 200
    period[1] = 0x32; // 50
  period[1] = 0x20;  // longer period
  period[2] = 0x03;  
    period[2] = 0;
  period[3] = 0;
  */

  char data[6]; 
	
  data[0] = 0xAA;
  data[1] = 0xB1;
  data[2] = period[3];  // 4byte number [3] is most sig. default is [1] to c8
  data[3] = period[2];
  data[4] = period[1];
  data[5] = period[0];
	
  return bulkTransfer(data);
}

/**.......................................................................
 * Set Burst Length
 */
int CbassBackend::setBurstLength(unsigned char* length)
{
  char data[6]; 
  data[0] = 0xAA;
  data[1] = 0xB2;
  data[2] = length[3];
  data[3] = length[2];
  data[4] = length[1];
  data[5] = length[0];
	
  return bulkTransfer(data);
}

/**.......................................................................
 * Setup ADCs
 */
int CbassBackend::setupAdc()
{
  char data[2]; 
  data[0] = 0xAA;
  data[1] = 0x80;
  
  return bulkTransfer(data);
}

/**.......................................................................
 * Set Bit
 */
int CbassBackend::setBit(unsigned char address, unsigned char bit, unsigned char value)
{
  char data[5]; 
  data[0] = 0xAA;
  data[1] = 0x81;
  data[2] = address;
  data[3] = bit;
  data[4] = value;
	
  return bulkTransfer(data);
}


/**.......................................................................
 * Stores data
 */
int CbassBackend::storeData(unsigned char address, unsigned char value)
{
  char data[4]; 
  data[0] = 0xAA;
  data[1] = 0x82;
  data[2] = address;
  data[3] = value;
  
  return bulkTransfer(data);
}

/**.......................................................................
 * Sets the Trigger
 */
int CbassBackend::trigger()
{
  char data[2]; 
  data[0] = 0xAA;
  data[1] = 0x40;
  
  return bulkTransfer(data);
}

/**.......................................................................
 * Aquires data
 */
int CbassBackend::acquireData(unsigned char enable)
{
  char data[3]; 
  data[0] = 0xAA;
  data[1] = 0x41;
  data[2] = enable;

  return bulkTransfer(data);
}

/**.......................................................................
 * Enable continuous mode
 */
int CbassBackend::enableContinuous(unsigned char enable)
{
  char data[3]; 
  data[0] = 0xAA;
  data[1] = 0x01;
  data[2] = enable;

  return bulkTransfer(data);
}

/**.......................................................................
 * Enable simulator
 */
int CbassBackend::enableSimulator(unsigned char enable)
{
  char data[3]; 
  data[0] = 0xAA;
  data[1] = 0x03;
  data[2] = enable;
	
  return bulkTransfer(data);
}

/**.......................................................................
 * Enable noise
 */
int CbassBackend::enableNoise(unsigned char enable)
{
  char data[3]; 
  data[0] = 0xAA;
  data[1] = 0x05;
  data[2] = enable;
	
  return bulkTransfer(data);
}


/**.......................................................................
 * Enable switch
 */
int CbassBackend::enableSwitch(unsigned char enable)
{
  COUT("ENABLE "<< enable);
  
  char data[3]; 
  COUT("sizeof(data) " << sizeof(data)); 
  data[0] = 0xAA; 
  data[1] = 0x07;
  //data[2] = enable;
  data[2] = 0;
  COUT("sizeof(data) " << sizeof(data)); 

	
  return bulkTransfer(data);
}

/**.......................................................................
 * Enable switch Alt
 */
int CbassBackend::enableSwitchAlt(unsigned char enable)
{
  char data[3]; 
  data[0] = 0xAA;
  data[1] = 0x09;
  data[2] = enable;
	
  return bulkTransfer(data);
}

/**.......................................................................
 * Get Data
 */
int CbassBackend::getData()
{
  int ret = 0;

#if DIR_HAVE_USB
  ret = usb_bulk_read(devHandle_, 0x88, data_, 512, BACKEND_TIMEOUT);
  // return value is number of bytes transferred
#endif

  return ret;
}


/*.......................................................................
 * Print the bits of a 1-byte unsigned char.
 */
void CbassBackend::printBits(unsigned char feature)
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


/*.......................................................................
 * Wrapper around the ubs function to bulk write
 */
int CbassBackend::bulkTransfer(char* data)
{
  int ret = -1;

#if DIR_HAVE_USB  
  ret = usb_bulk_write(devHandle_, 1, data, sizeof(data), BACKEND_TIMEOUT);
#endif

  return ret;
}

int CbassBackend::bulkTransfer(char* data, int size)
{
  int ret = -1;

#if DIR_HAVE_USB
  ret = usb_bulk_write(devHandle_, 1, data, size, BACKEND_TIMEOUT);
#endif

  return ret;
}


/*.......................................................................
 * Parse the response into an array of floats
 */
void CbassBackend::parseDataOld(int bytesTransferred)
{

  //  COUT("bytesTransferred: " << bytesTransferred);
  unsigned result;
  int i,j,k,m,n;
  float thisValue, thisChan, thisTime;
  char thisLine[130];
  int index = 1;
  int timeIndex = startIndex_;
  int chanIndex;
  bool valGood = false;

  for (j = 0; j < bytesTransferred / 16; j++) {
    // first we get the time value.
    // each sample has 16 bytes of 8 bits each.
    // first we have to convert each byte into their corresponding bits.
    index = 1;
    for (i = 0; i < 16; i+=2) {
      // turn the response into a string.
      for(k=7;k >= 0;k--) {
	result = data_[16*j+i+1] & (1U << k);
	if(index){
	  sprintf(thisLine, "%d", result ? 1 : 0);
	  index = 0;
	} else {
	  char poo[2];
	  sprintf(poo, "%d", result ? 1 : 0);
	  strcat(thisLine, poo);
	}
      }
      for(k=7;k >= 0;k--) {
	result = data_[16*j+i] & (1U << k);
	char poo[2];
	sprintf(poo, "%d", result ? 1 : 0);
	strcat(thisLine, poo);
      }
      
    }
    
    // parse the line itself.
    // first 26 bits are the timing signal.

    //    std::cout << "thisLine: " << thisLine << std::endl;

    // don't save the time until we determine that the value is good.
    thisTime = 0;
    for (k=0;k<26;k++){
      if(thisLine[k]=='1'){
	thisTime += pow(2, (25-k) );
      }
    }
    
    // rest are divided into 3 -- chan#, 14 -- data
    index = 26;
    for (m=0;m<6;m++){
      thisChan = 0;
      thisValue = 0;
      //      COUT("this chan: " << thisLine[index] << thisLine[index+1] << thisLine[index+2]);

      for (n=0;n<3;n++){
	if(thisLine[index+n]=='1'){
	  thisChan +=  pow(2, (2-n));
	}
      };

      // need to check that we have a decent channel
      if(thisChan>=0 & thisChan<=6){
	chanIndex = (int) thisChan;
	valGood = true;
      } else {
	valGood = false;
      };

      index = index+3;
      for (n=1;n<14;n++){
	if(thisLine[index+n]=='1'){
	  thisValue +=  pow(2, (13-n));
	}
      };
      if(thisLine[index] == '1'){
	thisValue -= pow(2,13);
      };
      index = index+14;
      //      COUT("index: " << index);
      //	std::cout << "thisChan" << thisChan << std::endl;
      thisValue = thisValue*convToV;
      
      //      std::cout << "thisValue " << thisValue << " valGood" << valGood <<  std::endl; 
      // if the value is good, we save the data.
      if(valGood){
	dataVals_[timeIndex][m] = thisValue;
	//	std::cout << "dataVals_[" << timeIndex << "][" << chanIndex << "] = " << dataVals_[timeIndex][chanIndex] << std::endl;

      }
    }
    if(valGood){
      // convert to actual seconds -- and fill in the buffer
      thisTime = thisTime*20e-9;
      timeVals_[timeIndex] = thisTime;
    }
    timeIndex+=1;
  };
  // by having the indices this way, we can populate the whole vector and then write it out once per second.
  // and know where we are in populating.  this won't be necessary once i have a share object that i'm filling in.
  currentIndex_ = timeIndex;
  startIndex_ = currentIndex_;
  
  return;
}    

/*.......................................................................
 * Parse the response into an array of floats
 */
void CbassBackend::parseData1(int bytesTransferred)
{

  unsigned result;
  int i,j,k,m,n;
  float thisValue, thisChan, thisTime;
  char thisLine[130];
  char thisSample[15];
  int index = 1;
  int timeIndex = startIndex_;
  int chanIndex;
  bool valGood = false;
  float flag;
  char actualSum, desiredSum;
  
  for (j = 0; j < bytesTransferred / 16; j++) {
    // first, let's get the values of the sum;
    actualSum = data_[16*j];
    for (i=1;i<15;i++){
      actualSum ^= data_[16*j+i];
    };
    desiredSum = data_[16*j+15];

    // next, the time value;
    // each sample has 16 bytes of 8 bits each.
    // first we have to convert each byte into their corresponding bits.
    index = 1;
    for (i = 0; i < 16; i+=2) {
      // turn the response into a string.
      for(k=7;k >= 0;k--) {
	result = data_[16*j+i+1] & (1U << k);
	if(index==1){
	  sprintf(thisLine, "%d", result ? 1 : 0);
	  index = 0;
	} else {
	  char poo[2];
	  sprintf(poo, "%d", result ? 1 : 0);
	  strcat(thisLine, poo);
	}
      }
      for(k=7;k >= 0;k--) {
	result = data_[16*j+i] & (1U << k);
	char poo[2];
	sprintf(poo, "%d", result ? 1 : 0);
	strcat(thisLine, poo);
      }
      
    }

    // parse the line itself.
    // first 26 bits are the timing signal.

    // parse the time and put it in our structure.
    thisTime = 0;
    for (k=0;k<26;k++){
      if(thisLine[k]=='1'){
	thisTime += pow(2, (25-k) );
      }
    }
    thisTime = thisTime*20e-9; // convert to seconds.
    timeVals_[timeIndex] = thisTime;

    // next 84 (14*6) are the data themselves.
    index = 26;
    for (m=0;m<6;m++){
      thisValue = 0;
      
      for (n=1;n<14;n++){
	if(thisLine[index+n]=='1'){
	  thisValue +=  pow(2, (13-n));
	}
      };
      if(thisLine[index] == '1'){
	thisValue -= pow(2,13);
      };
      index = index+14;
      thisValue = thisValue*convToV;

      dataVals_[timeIndex][m] = thisValue;
    };

    // bits 17-12 correspond to (switch_alt, switch_en, noise_on,
    // simulate_en, cont_mode, nonlin_en) bits 
    // 11-8 are (fifo empty, rollover_flag of 1PPS, dcm_locked if
    // clock is good, and pps_sync)

    flag = 0;
    for (m=0;m<10; m++) {
      if(thisLine[index+m]=='1'){
	// flag is set
	flag += pow(2, m);
      };
      
      // check to make sure we're not in burst mode
      if(m==4){
	if(thisLine[index+m]=='0'){
	  burst_ = true;
	} else {
	  burst_ = false;
	};
      };	
    };
    
    // last term in the flag is a checksum of the bytes.  we've
    // already calculated the desired and the actual, so we jsut
    // compare.
    if(!actualSum==desiredSum){
      flag += pow(2,13);
    };

    // set the flags structure
    flags_[timeIndex] = (unsigned short) flag;

    // increment our time index
    timeIndex+=1;
  };
  // by having the indices this way, we can populate the whole vector and then write it out once per second.
  // and know where we are in populating.  this won't be necessary once i have a share object that i'm filling in.
  currentIndex_ = timeIndex;
  startIndex_ = currentIndex_;
  
  return;
}


/*.......................................................................
 * Parse the response into an array of floats
 */
void CbassBackend::parseData2(int bytesTransferred)
{

  // This is going to be a fucking mess.  fucking mess, i tell you.

  unsigned result;
  int i,j,k,m,n;
  float thisValue, thisChan, thisTime, packetIndex;
  char thisLine[130];
  char thisSample[15];
  int index = 1;
  int arrayIndex_ = 0;
  int chanIndex;
  int packIndex;
  bool valGood = false;
  float flag;
  char actualSum, desiredSum;

  // modifications:  
  // arrayIndex_ can't go up unless the time has actually changed.
  // we fill in the channels until the time index has changed.
  
  for (j = 0; j < bytesTransferred / 16; j++) {

    // each sample has 16 bytes of 8 bits each.
    // first we have to convert each byte into their corresponding bits.
    index = 1;
    for (i = 0; i < 16; i+=2) {
      // turn the response into a string.
      for(k=7;k >= 0;k--) {
	result = data_[16*j+i+1] & (1U << k);
	if(index==1){
	  sprintf(thisLine, "%d", result ? 1 : 0);
	  index = 0;
	} else {
	  char poo[2];
	  sprintf(poo, "%d", result ? 1 : 0);
	  strcat(thisLine, poo);
	}
      }
      for(k=7;k >= 0;k--) {
	result = data_[16*j+i] & (1U << k);
	char poo[2];
	sprintf(poo, "%d", result ? 1 : 0);
	strcat(thisLine, poo);
      }
      
    }

    // parse the line itself.

    // first 26 bits are the timing signal.

    // parse the time and put it in our structure.
    thisTime = 0;
    for (k=0;k<26;k++){
      if(thisLine[k]=='1'){
	thisTime += pow(2, (25-k) );
      }
    }
    thisTime = thisTime*20e-9; // convert to seconds.
    timeVals_[arrayIndex_] = thisTime;

    // next 84 (14*6) are the data themselves.
    index = 26;
    for (m=0;m<6;m++){
      thisValue = 0;
      
      for (n=1;n<14;n++){
	if(thisLine[index+n]=='1'){
	  thisValue +=  pow(2, (13-n));
	}
      };
      if(thisLine[index] == '1'){
	thisValue -= pow(2,13);
      };
      index = index+14;
      thisValue = thisValue*convToV;

      dataVals_[arrayIndex_][5-m] = thisValue;  
      // the 5-m is because the orders are now switched, whereas
      // before we reported channels 1-6, now we're reporting them in
      // backwards order.
    };


    // bits 17-12 correspond to (switch_alt, switch_en, noise_on,
    // simulate_en, cont_mode, nonlin_en) bits 
    // 11-8 are (fifo empty, rollover_flag of 1PPS, dcm_locked if
    // clock is good, and pps_sync)

    flag = 0;
    for (m=0;m<10; m++) {
      if(thisLine[index+m]=='1'){
	// flag is set
	flag += pow(2, m);
      };
      
      // check to make sure we're not in burst mode
      if(m==4){
	if(thisLine[index+m]=='0'){
	  burst_ = true;
	} else {
	  burst_ = false;
	};
      };	
    };
    index += 10;  // increment beyond the flags


    // next 4 bits are the packet number
    for (m=3;m>=0;m--) {
      if(thisLine[index+m]=='1') {
	packetVals_[arrayIndex_] = 3-m;
      };
    };
    index += 4;

    // last four bits are the checksum, which is fucking retarded
    // because checksum should be of bytes, and not bits. But
    // whatever, most of what's going on here is retarded anyway.



    // set the flags structure
    flags_[arrayIndex_] = (unsigned short) flag;

    // increment our time index
    arrayIndex_+=1;
  };

  // now we have to sort all the data that we have.
  sortData(arrayIndex_);

  return;
}


/*.......................................................................
 * Parse the response into an array of floats - version for Feb 2010
 * backend code.
 */
void CbassBackend::parseData(int bytesTransferred)
{

  unsigned result;
  int i,k,m,n;
  int intNum, packNum;
  float fifo_backlog, switch_period, trim_length, integration_shortfall;
  float thisValue, thisTime;
  char thisLine[130];
  int index = 1;
  int arrayIndex_ = 0;
  float flag;
  
#if(0)
  // debugging to test the backend issue.
  ofstream outputdata;
  std::string file = "/opt/data/cbass/burst/cont_test2";
  outputdata.open(file.c_str(), ios_base::app);
  if(!outputdata.is_open()) {
    // could not open file
    COUT("Error: File could not be opened");
    return;
  };
#endif



  int timeIndex = startIndex_;

  /* First things first, if we don't get 400 bytes, don't parse */
  if(bytesTransferred != 400){
    CTOUT("Bytes received: " << bytesTransferred);
    ReportError("Number of bytes received not equal to expected: " << bytesTransferred << " bytes received");

    return;
  }

  /* Each transfer has 5 integrations of 5, 128-bit (16-byte) packets each */
  for (intNum=0; intNum<5; intNum++){

    for (packNum=0; packNum<5; packNum++) {

      index = 1;
      for (i=0; i<16; i+=2) {
	// each sample has 16 bytes of 8 bits each.
	// first we have to convert each byte into their corresponding bits.
	// turn the response into a string.
	for(k=7;k >= 0;k--) {
	  result = data_[intNum*5*16+packNum*16+i+1] & (1U << k);
	  if(index==1){
	    sprintf(thisLine, "%d", result ? 1 : 0);
	    index = 0;
	  } else {
	    char poo[2];
	    sprintf(poo, "%d", result ? 1 : 0);
	    strcat(thisLine, poo);
	  }
	}
	for(k=7;k >= 0;k--) {
	  result = data_[intNum*5*16+packNum*16+i] & (1U << k);
	  char poo[2];
	  sprintf(poo, "%d", result ? 1 : 0);
	  strcat(thisLine, poo);
	}
      }  // thisLine should be the packet.
      
      // now we parse the line
      //      COUT("packnum: " << packNum << "thisLIne: " << thisLine);
      //outputdata << thisLine << endl;


      switch(packNum){
      case 0:
	// first packet is broken down with time/ch2H/ch2L/ch1H/ch1L/diagnostics
	// first 26 bits are the timing signal.
	thisTime = 0;
	for (k=0;k<26;k++){
	  if(thisLine[k]=='1'){
	    thisTime += pow(2, (25-k) );
	  }
	}
	thisTime = thisTime*20e-9; // convert to seconds.
	sortTimeVals_[timeIndex] = thisTime;

	//	COUT("thisTime: " <<thisTime);
	// from 101-6, it's the data itself in 24-bit numbers
	index = 26;
	for (m=0;m<4;m++){
	  thisValue = 0;
	  
	  for (n=1;n<24;n++){
	    if(thisLine[index+n]=='1'){
	      thisValue +=  pow(2, (23-n));
	    }
	  };
	  if(thisLine[index] == '1'){
	    thisValue -= pow(2,23);
	  };
	  index = index+24;
	  thisValue = thisValue*convToV;

	  //	  COUT("thisValue: " << thisValue);
	  sortDataVals_[timeIndex][3-m] = thisValue;  
	  // the 3-m is because we do ch2H/ch2L/ch1H/ch1L
	};

	// bits 5-0 correspond to (switch_alt, switch_en, noise_on,
	// simulate_en, cont_mode, nonlin_en) bits 
	flag = 0;
	for (m=0;m<5; m++) {
	  if(thisLine[index+m]=='1'){
	    // flag is set
	    flag += pow(2, m);
	  };
	  
	  // check to make sure we're not in burst mode
	  if(m==4){
	    if(thisLine[index+m]=='0'){
	      burst_ = true;
	    } else {
	      burst_ = false;
	    };
	  };	
	};
	break;  // done with first packet.
      
      case 1:
	// this pack has format ch5L/ch4H/ch4L/ch3H/ch3L/fifo_backlog/more diagnostics
	index = 0;
	for (m=0;m<5;m++){
	  thisValue = 0;
	  
	  for (n=1;n<24;n++){
	    if(thisLine[index+n]=='1'){
	      thisValue +=  pow(2, (23-n));
	    }
	  };
	  if(thisLine[index] == '1'){
	    thisValue -= pow(2,23);
	  };
	  index = index+24;
	  thisValue = thisValue*convToV;
	  
	  sortDataVals_[timeIndex][8-m] = thisValue;  
	  // the 8-m is because we're in reverse order and we already
	  // have 4 entries populated
	};
	
	// bits 7-4 are the fifo backlog
	fifo_backlog = 0;
	for (m=0;m<4; m++) {
	  if(thisLine[index+m]=='1'){
	    fifo_backlog += pow(2, (3-m));
	  };
	};
	index = index+4;

	// bits 3-0 are fifo_empty, rollover_flag, dcm_locked, pps_sync;
	for (m=0;m<4; m++) {
	  if(thisLine[index+m]=='1'){
	    // flag is set
	    flag += pow(2, m+6); // we already have the first 6 bits set from packet 1.
	  };	
	};

	break;  // done with second packet

      case 2:
	// this pack has format ch7H/ch7L/ch6H/ch6L/ch5H/switch_period
	index = 0;
	for (m=0;m<5;m++){
	  thisValue = 0;
	  
	  for (n=1;n<24;n++){
	    if(thisLine[index+n]=='1'){
	      thisValue +=  pow(2, (23-n));
	    }
	  };
	  if(thisLine[index] == '1'){
	    thisValue -= pow(2,23);
	  };
	  index = index+24;
	  thisValue = thisValue*convToV;
	  
	  sortDataVals_[timeIndex][13-m] = thisValue;  
	  // the 13-m is because we're in reverse order and we already
	  // have 9 entries populated
	};

	// bits 7-0 are the switch period
	switch_period = 0;
	for (m=0;m<7; m++) {
	  if(thisLine[index+m]=='1'){
	    switch_period += pow(2, (7-m));
	  };
	};
	// that's the end of the third pack

      case 3:
	// this pack, in same format as previous, has:
	// ch10L/ch9H/ch9L/ch8H/ch8L/trim_length
	index = 0;
	for (m=0;m<5;m++){
	  thisValue = 0;
	  
	  for (n=1;n<24;n++){
	    if(thisLine[index+n]=='1'){
	      thisValue +=  pow(2, (23-n));
	    }
	  };
	  if(thisLine[index] == '1'){
	    thisValue -= pow(2,23);
	  };
	  index = index+24;
	  thisValue = thisValue*convToV;
	  
	  sortDataVals_[timeIndex][18-m] = thisValue;  
	  // the 18-m is because we're in reverse order and we already
	  // have 13 entries populated
	};

	// bits 7-0 are the trim_length
	trim_length = 0;
	for (m=0;m<7; m++) {
	  if(thisLine[index+m]=='1'){
	    trim_length += pow(2, (7-m));
	  };
	};
	// that's the end of the fourth pack

      case 4:
	// this pack, in same format as previous, has:
	// ch12H/ch12L/ch11H/ch11L/ch10H/integration_shortfall
	index = 0;
	for (m=0;m<5;m++){
	  thisValue = 0;
	  
	  for (n=1;n<24;n++){
	    if(thisLine[index+n]=='1'){
	      thisValue +=  pow(2, (23-n));
	    }
	  };
	  if(thisLine[index] == '1'){
	    thisValue -= pow(2,23);
	  };
	  index = index+24;
	  thisValue = thisValue*convToV;
	  
	  sortDataVals_[timeIndex][23-m] = thisValue;  
	  // the 23-m is because we're in reverse order and we already
	  // have 18 entries populated
	};

	// bits 7-0 are the integration_shortfall
	integration_shortfall = 0;
	for (m=0;m<7; m++) {
	  if(thisLine[index+m]=='1'){
	    integration_shortfall += pow(2, (7-m));
	  };
	};
	// that's the end of the fifth pack

      }  // done with switch statement.

    } // done with loop over pack numbers

    // set the flags structure
    sortFlags_[timeIndex] = (unsigned short) flag;

    // set the diagnostics structure
    sortDiagnostics_[timeIndex][0] = fifo_backlog;
    sortDiagnostics_[timeIndex][1] = switch_period;
    sortDiagnostics_[timeIndex][2] = trim_length;
    sortDiagnostics_[timeIndex][3] = integration_shortfall;

#if(0)
    // print out for debugging
    COUT("time index: " << timeIndex);
    COUT("time returned: " << sortTimeVals_[timeIndex]);
    COUT("values returned");
    for (m=0;m<24;m++){
      COUT("sortDataVals_[timeIndex][" << m << "]: " << sortDataVals_[timeIndex][m]);
    }
    COUT("flags: " << sortFlags_[timeIndex]);
    COUT("fifo_back: " << fifo_backlog);
    COUT("switch period: " << switch_period);
    COUT("trim_length: " << trim_length);
    COUT("int_shortfall: " << integration_shortfall);

#endif

    
    // increment our time index;
    timeIndex++;
    // done with integration loop.  should all be parsed.
  };
  numFrames_ = timeIndex;

#if(0)
  outputdata.close();
#endif


  return;
}



void CbassBackend::sortData(int numSamples)
{
  // This function should sort our data and put it in the same output
  // format as the previous parseData1 function did.
  
  int timeIndex = startIndex_;
  int m,n,o;  
  float thisTime;
  int numTimes;

  // First let's figure out the number of distinct time samples
  for (m=0;m<numSamples;m++){
    if(m==0){
      thisTime = timeVals_[0];
      numTimes = 1;
    } else {
      if(timeVals_[m] != thisTime) {
	numTimes++;
	thisTime = timeVals_[m];
      };
    }
  }
  if(numSamples % 4 != 0){
    COUT("Fuck.  have to do all the logic");
  };
  // ok.  now we sort through them. They should always come in
  // multiples of 4.  Let's hope that's true, or else we'll need some
  // better logic here.
  
  for (m=0;m<numTimes;m++){
    // get the times and flags done
    sortTimeVals_[timeIndex] = timeVals_[4*m];
    sortFlags_[timeIndex] = flags_[4*m]; 
    
    for (n=0;n<4;n++){
      // this is where we cycle through the packet values.

      for(o=0;o<6;o++){
	sortDataVals_[timeIndex][6*( (int) packetVals_[4*m+n]) + o] = dataVals_[4*m+n][o];
      };

    };
    timeIndex++;

  };
  currentIndex_ = timeIndex;
  startIndex_ = currentIndex_;

  return;

};



/*.......................................................................
 * Parse the response into an array of floats - version for Sep 2011
 * backend code.
 */
void CbassBackend::parseData2011(int bytesTransferred)
{

  unsigned result;
  int i,k,m,n;
  int intNum, packNum;
  float fifo_backlog, switch_period, trim_length, integration_shortfall;
  float backend_version, avg_sec, reserved;
  float thisValue, thisTime;
  double thisAvgSec;
  char thisLine[130];
  int index = 1;
  int thisIndex = 0, coeffIndex;
  int arrayIndex_ = 0;
  float flag;
  float coeffVals[28];

#if(0)
  // debugging to test the backend issue.
  ofstream outputdata;
  std::string file = "/opt/data/cbass/burst/cont_test2";
  outputdata.open(file.c_str(), ios_base::app);
  if(!outputdata.is_open()) {
    // could not open file
    COUT("Error: File could not be opened");
    return;
  };
#endif

  int timeIndex = startIndex_;

  /* First things first, if we don't get 400 bytes, don't parse */
  if(bytesTransferred != 512){
    CTOUT("Bytes received: " << bytesTransferred);
    ReportError("Number of bytes received not equal to expected: " << bytesTransferred << " bytes received");

    return;
  }

  /* Each transfer has 4 integrations of 8, 128-bit (16-byte) packets each */
  for (intNum=0; intNum<4; intNum++){

    for (packNum=0; packNum<8; packNum++) {

      index = 1;
      for (i=0; i<16; i+=2) {
	// each sample has 16 bytes of 8 bits each.
	// first we have to convert each byte into their corresponding bits.
	// turn the response into a string.
	for(k=7;k >= 0;k--) {
	  result = data_[intNum*8*16+packNum*16+i+1] & (1U << k);
	  if(index==1){
	    sprintf(thisLine, "%d", result ? 1 : 0);
	    index = 0;
	  } else {
	    char poo[2];
	    sprintf(poo, "%d", result ? 1 : 0);
	    strcat(thisLine, poo);
	  }
	}
	for(k=7;k >= 0;k--) {
	  result = data_[intNum*8*16+packNum*16+i] & (1U << k);
	  char poo[2];
	  sprintf(poo, "%d", result ? 1 : 0);
	  strcat(thisLine, poo);
	}
      }  // thisLine should be the packet.
      
      // now we parse the line
      //      COUT("packnum: " << packNum << "thisLIne: " << thisLine);
      //outputdata << thisLine << endl;


      switch(packNum){
      case 0:
	// first packet is broken down with time/ch2H/ch2L/ch1H/ch1L/diagnostics
	// first 26 bits are the timing signal.
	thisTime = 0;
	for (k=0;k<26;k++){
	  if(thisLine[k]=='1'){
	    thisTime += pow(2, (25-k) );
	  }
	}
	thisTime = thisTime*20e-9; // convert to seconds.
	sortTimeVals_[timeIndex] = thisTime;

	//	COUT("thisTime: " <<thisTime);
	// from 101-6, it's the data itself in 24-bit numbers
	index = 26;
	for (m=0;m<4;m++){
	  thisValue = 0;
	  
	  for (n=1;n<24;n++){
	    if(thisLine[index+n]=='1'){
	      thisValue +=  pow(2, (23-n));
	    }
	  };
	  if(thisLine[index] == '1'){
	    thisValue -= pow(2,23);
	  };
	  index = index+24;
	  thisValue = thisValue*convToV;

	  //	  COUT("thisValue: " << thisValue);
	  sortDataVals_[timeIndex][3-m] = thisValue;  
	  // the 3-m is because we do ch2H/ch2L/ch1H/ch1L
	};

	// bits 5-0 correspond to (switch_alt, switch_en, noise_on,
	// simulate_en, cont_mode, nonlin_en) bits 
	flag = 0;
	for (m=0;m<5; m++) {
	  if(thisLine[index+m]=='1'){
	    // flag is set
	    flag += pow(2, m);
	  };
	  
	  // check to make sure we're not in burst mode
	  if(m==4){
	    if(thisLine[index+m]=='0'){
	      burst_ = true;
	    } else {
	      burst_ = false;
	    };
	  };	
	};
	break;  // done with first packet.
      
      case 1:
	// this pack has format ch5L/ch4H/ch4L/ch3H/ch3L/fifo_backlog/more diagnostics
	index = 0;
	for (m=0;m<5;m++){
	  thisValue = 0;
	  
	  for (n=1;n<24;n++){
	    if(thisLine[index+n]=='1'){
	      thisValue +=  pow(2, (23-n));
	    }
	  };
	  if(thisLine[index] == '1'){
	    thisValue -= pow(2,23);
	  };
	  index = index+24;
	  thisValue = thisValue*convToV;
	  
	  sortDataVals_[timeIndex][8-m] = thisValue;  
	  // the 8-m is because we're in reverse order and we already
	  // have 4 entries populated
	};
	
	// bits 7-4 are the fifo backlog
	fifo_backlog = 0;
	for (m=0;m<4; m++) {
	  if(thisLine[index+m]=='1'){
	    fifo_backlog += pow(2, (3-m));
	  };
	};
	index = index+4;

	// bits 3-0 are fifo_empty, rollover_flag, dcm_locked, pps_sync;
	for (m=0;m<4; m++) {
	  if(thisLine[index+m]=='1'){
	    // flag is set
	    flag += pow(2, m+6); // we already have the first 6 bits set from packet 1.
	  };	
	};

	break;  // done with second packet

      case 2:
	// this pack has format ch7H/ch7L/ch6H/ch6L/ch5H/switch_period
	index = 0;
	for (m=0;m<5;m++){
	  thisValue = 0;
	  
	  for (n=1;n<24;n++){
	    if(thisLine[index+n]=='1'){
	      thisValue +=  pow(2, (23-n));
	    }
	  };
	  if(thisLine[index] == '1'){
	    thisValue -= pow(2,23);
	  };
	  index = index+24;
	  thisValue = thisValue*convToV;
	  
	  sortDataVals_[timeIndex][13-m] = thisValue;  
	  // the 13-m is because we're in reverse order and we already
	  // have 9 entries populated
	};

	// bits 7-0 are the switch period
	switch_period = 0;
	for (m=0;m<8; m++) {
	  if(thisLine[index+m]=='1'){
	    switch_period += pow(2, (7-m));
	  };
	};
	break; // that's the end of the third pack

      case 3:
	// this pack, in same format as previous, has:
	// ch10L/ch9H/ch9L/ch8H/ch8L/trim_length
	index = 0;
	for (m=0;m<5;m++){
	  thisValue = 0;
	  
	  for (n=1;n<24;n++){
	    if(thisLine[index+n]=='1'){
	      thisValue +=  pow(2, (23-n));
	    }
	  };
	  if(thisLine[index] == '1'){
	    thisValue -= pow(2,23);
	  };
	  index = index+24;
	  thisValue = thisValue*convToV;
	  
	  sortDataVals_[timeIndex][18-m] = thisValue;  
	  // the 18-m is because we're in reverse order and we already
	  // have 13 entries populated
	};

	// bits 7-0 are the trim_length
	trim_length = 0;
	for (m=0;m<8; m++) {
	  if(thisLine[index+m]=='1'){
	    trim_length += pow(2, (7-m));
	  };
	};
	break; // that's the end of the fourth pack

      case 4:
	// this pack, in same format as previous, has:
	// ch12H/ch12L/ch11H/ch11L/ch10H/integration_shortfall
	index = 0;
	for (m=0;m<5;m++){
	  thisValue = 0;
	  
	  for (n=1;n<24;n++){
	    if(thisLine[index+n]=='1'){
	      thisValue +=  pow(2, (23-n));
	    }
	  };
	  if(thisLine[index] == '1'){
	    thisValue -= pow(2,23);
	  };
	  index = index+24;
	  thisValue = thisValue*convToV;
	  
	  sortDataVals_[timeIndex][23-m] = thisValue;  
	  // the 23-m is because we're in reverse order and we already
	  // have 18 entries populated
	};

	// bits 7-0 are the integration_shortfall
	integration_shortfall = 0;
	for (m=0;m<8; m++) {
	  if(thisLine[index+m]=='1'){
	    integration_shortfall += pow(2, (7-m));
	  };
	};
	break; // that's the end of the fifth pack


      case 5:
	// timestamp/ch5-ch6/ch3-ch4/ch1-ch2/backend number

	// first 26 bits are the timing signal.
	thisTime = 0;
	for (k=0;k<26;k++){
	  if(thisLine[k]=='1'){
	    thisTime += pow(2, (25-k) );
	  }
	}
        
	thisTime = thisTime*20e-9; // convert to seconds.
	sortTimeVals2_[timeIndex] = thisTime;
	// from 101-12, it's the data itself in 30-bit numbers
	index = 26;
	for (m=0;m<3;m++){
	  thisValue = 0;
	  
	  for (n=1;n<30;n++){
	    if(thisLine[index+n]=='1'){
	      thisValue +=  pow(2, (29-n));
	    }
	  };
	  if(thisLine[index] == '1'){
	    thisValue -= pow(2,29);
	  };
	  index = index+30;
	  thisValue = thisValue*convToV2;

	  sortRegData_[timeIndex][2-m] = thisValue;  
	};


	// bits 11-0 are the backend version number
	backend_version = 0;
	for (m=0;m<12; m++) {
	  if(thisLine[index+m]=='1'){
	    backend_version += pow(2, (11-m));
	  };
	};
	sortBackendVersion_[timeIndex] = backend_version;
	break; // that's the end of the sixth pack

      case 6:
	// avg sec length/ch11-ch12/ch9-ch10/ch7-ch8/RESERVED

	//	COUT("this line: " << thisLine);
	// first 26 bits are the avg sec length
	thisAvgSec = 0;
	for (k=0;k<26;k++){
	  if(thisLine[k]=='1'){
	    thisAvgSec += pow(2, (25-k) );
	  }
	}
	//	thisTime = thisTime;//*20e-9; // convert to seconds.
	sortAvgSec_[timeIndex] = thisAvgSec;
	//	std::cout << "avgsec value " << thisAvgSec << endl;
	//	COUT("avgsec: " << thisAvgSec - 4.99997e+07);
	//	COUT("sortAvgSec_: " << sortAvgSec_[timeIndex] - 4.99997e+07);



	// from 101-12, it's the data itself in 30-bit numbers
	index = 26;
	for (m=0;m<3;m++){
	  thisValue = 0;
	  
	  for (n=1;n<30;n++){
	    if(thisLine[index+n]=='1'){
	      thisValue +=  pow(2, (29-n));
	    }
	  };
	  if(thisLine[index] == '1'){
	    thisValue -= pow(2,29);
	  };
	  index = index+30;
	  thisValue = thisValue*convToV2;

	  sortRegData_[timeIndex][5-m] = thisValue;  
	};


	// bits 11-0 are reserved
	reserved = 0;
	for (m=0;m<12; m++) {
	  if(thisLine[index+m]=='1'){
	    reserved += pow(2, (11-m));
	  };
	};
	break; // that's the end of the seventh pack

      case 7:
	// this one's a pain as it cycles through all the coefficients
	// c7/c6/c5/c4/c3/c2/c1/index

	// let's check the last index first:
	index = 126;
	thisIndex = 0;
	for (n=0;n<2;n++){
	  if(thisLine[index+n]=='1'){
	    thisIndex += pow(2,(1-n));
	  }
	}
	// next we go back to read in the rest.
	index = 0;
	for (m=0;m<7;m++){
	  thisValue = 0;
	  
	  for (n=1;n<18;n++){
	    if(thisLine[index+n]=='1'){
	      thisValue +=  pow(2, (17-n));
	    }
	  };
	  if(thisLine[index] == '1'){
	    thisValue -= pow(2,17);
	  };
	  index = index+18;
	  // no conversion factor
	  thisValue = thisValue;

	  // first we just write them to the coeffVals, once we know
	  // the index, we can write it to the sorted registers
	  coeffIndex = (int) 7*(thisIndex+1)-1;
	  coeffVals[coeffIndex - m] = thisValue;
	}
	break;

      }  // done with switch statement.

    } // done with loop over pack numbers

    // set the flags structure
    sortFlags_[timeIndex] = (unsigned short) flag;

    // set the diagnostics structure
    sortDiagnostics_[timeIndex][0] = fifo_backlog;
    sortDiagnostics_[timeIndex][1] = switch_period;
    sortDiagnostics_[timeIndex][2] = trim_length;
    sortDiagnostics_[timeIndex][3] = integration_shortfall;


    if(intNum==3){
      // next we have to parse all our coefficient values.
      for (m=0;m<24;m++){
	sortNonlin_[timeIndex][m] = coeffVals[m];
      };

      for (m=24;m<28;m++){
	sortAlpha_[timeIndex][m-24] = coeffVals[m];
      };
    };

    //    COUT("fifo_back: " << fifo_backlog);

#if(0)
    // print out for debugging
    COUT("time index: " << timeIndex);
    COUT("time returned: " << sortTimeVals_[timeIndex]);
    COUT("values returned");
    for (m=0;m<24;m++){
      COUT("sortDataVals_[timeIndex][" << m << "]: " << sortDataVals_[timeIndex][m]);
    }
    COUT("flags: " << sortFlags_[timeIndex]);
    COUT("switch period: " << switch_period);
    COUT("trim_length: " << trim_length);
    COUT("int_shortfall: " << integration_shortfall);

    for (m=0;m<6;m++){
      COUT("sortRegData_[timeIndex][" << m << "]: " << sortRegData_[timeIndex][m]);
    };


    if(timeIndex==3){
    for (m=0;m<24;m++){
      COUT("sortNonlin_[timeIndex][" << m << "]: " << sortNonlin_[timeIndex][m]);
    };

    }

    for (m=0;m<4;m++){
      COUT("sortAlpha_[timeIndex][" << m << "]: " << sortAlpha_[timeIndex][m]);
    }

    COUT("BACKEND VERSION: " << backend_version);
#endif

    
    // increment our time index;
    timeIndex++;
    // done with integration loop.  should all be parsed.
  };
  numFrames_ = timeIndex;

#if(0)
  outputdata.close();
#endif


  return;
}
