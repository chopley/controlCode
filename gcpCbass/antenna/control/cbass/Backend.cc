#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <iostream>
#include <fstream>
#include <cstdlib>


#include "gcp/control/code/unix/libunix_src/common/tcpip.h"
#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/LogFile.h"
#include "gcp/util/common/TimeVal.h"
#include "gcp/util/specific/CbassBackend.h"

#include "gcp/antenna/control/specific/AntennaException.h"
#include "gcp/antenna/control/specific/Backend.h"
#include "gcp/antenna/control/specific/SpecificShare.h"

#include <fcntl.h>

using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

#define acceptDelta 0.001

/**.......................................................................
 * Constructor with shared resources.
 */
Backend::Backend(SpecificShare* share, string name) : 
  Board(share, name), timeBuffer_(MAX_BUFFER_LENGTH), flagBuffer_(MAX_BUFFER_LENGTH), dataBuffer_(MAX_BUFFER_LENGTH, std::vector<float>(NUM_RECEIVER_SWITCH_CHANNELS)), diagnosticsBuffer_(MAX_BUFFER_LENGTH, std::vector<float>(NUM_RECEIVER_DIAGNOSTICS)) ,versionBuffer_(MAX_BUFFER_LENGTH), avgSecBuffer_(MAX_BUFFER_LENGTH), regDataBuffer_(MAX_BUFFER_LENGTH, std::vector<float>(NUM_RECEIVER_CHANNELS)), alphaBuffer_(MAX_BUFFER_LENGTH, std::vector<float>(NUM_ALPHA_CORRECTIONS)), nonlinBuffer_(MAX_BUFFER_LENGTH, std::vector<float>(NUM_RECEIVER_SWITCH_CHANNELS))
{
  connected_ = false;
  currentIndex_ = 0;
  prevSecStart_ = 0;
  prevSecEnd_   = 0;
  thisSecStart_ = 0;
  prevTime_     = 0;
  missedCommCounter_ = 0;

  rxUtc_         = 0;
  rxData_        = 0;
  rxFlags_       = 0;
  rxDiagnostics_ = 0;
  rxVersion_     = 0;
  rxSecLength_   = 0;
  rxNonlin_      = 0;
  rxAlpha_       = 0;

  rxUtc_  = findReg("utc");
  rxData_ = findReg("data");
  rxFlags_= findReg("flags");
  rxSwitchData_  = findReg("switchData");
  rxDiagnostics_ = findReg("diagnostics");
  rxVersion_     = findReg("dbeVersion");
  rxSecLength_   = findReg("avgSecond");
  rxNonlin_      = findReg("nonlin");
  rxAlpha_       = findReg("alpha");
}

/**.......................................................................
 * Constructor with no shared resources.
 */
 Backend::Backend() : 
   Board(), timeBuffer_(MAX_BUFFER_LENGTH), flagBuffer_(MAX_BUFFER_LENGTH), dataBuffer_(MAX_BUFFER_LENGTH, std::vector<float>(NUM_RECEIVER_CHANNELS)),diagnosticsBuffer_(MAX_BUFFER_LENGTH, std::vector<float>(NUM_RECEIVER_DIAGNOSTICS)), versionBuffer_(MAX_BUFFER_LENGTH), avgSecBuffer_(MAX_BUFFER_LENGTH), regDataBuffer_(MAX_BUFFER_LENGTH, std::vector<float>(NUM_RECEIVER_CHANNELS)), alphaBuffer_(MAX_BUFFER_LENGTH, std::vector<float>(NUM_ALPHA_CORRECTIONS)), nonlinBuffer_(MAX_BUFFER_LENGTH, std::vector<float>(NUM_RECEIVER_SWITCH_CHANNELS))
{
  connected_ = false;
  share_     = 0;
  currentIndex_ = 0;
  prevSecStart_ = 0;
  prevSecEnd_   = 0;
  thisSecStart_ = 0;
  prevTime_     = 0;
}

/**.......................................................................
 * Destructor
 */
Backend::~Backend()
{
  // Disconnect from the Backend

  if(connected_){
    disconnect();
  };

}

/**.......................................................................
 * Function to connect to the backend
 */
void Backend::connect()
{
  // first we check if we're connected
  if(!connected_){
    // put try catch here
    try{
      cbassBackend_.backendConnect();
    } catch (...) {
      connected_ = false;
      COUT("Error connecting to backend");
      return;
    };
  };

  // load the hex file. -- could do a fpga reset/fifo reset, but
  // there's no way to know the code is on the box.  loading the hex
  // does both of these.
  try{
    cbassBackend_.loadHex();
  } catch (...) {
    connected_ = false;
    COUT("Error downloading hex file");
    return;
  };

  connected_ = true;

  // default to alternate switching
  unsigned char val = 1;
  issueCommand(gcp::util::CbassBackend::ENABLE_WALSH_ALT, val);  

  // default to a switch period of 2ms.
  //  val = 32;
  //  issueCommand(gcp::util::CbassBackend::SET_SWITCH_PERIOD, val);    
  
  // set the trim length to 75
  val = 0;
  issueCommand(gcp::util::CbassBackend::WALSH_TRIM_LENGTH, val);      

  // reset the fpga, fifo
  issueCommand(gcp::util::CbassBackend::FPGA_RESET);  
  issueCommand(gcp::util::CbassBackend::FIFO_RESET);

  // enable continues, acquire data
  val = 1;
  issueCommand(gcp::util::CbassBackend::ENABLE_CONTINUOUS, val);
  issueCommand(gcp::util::CbassBackend::ACQUIRE_DATA, val);  

  // set the simulator on
  //  issueCommand(gcp::util::CbassBackend::ENABLE_SIMULATOR, val);


  return;
}

/**.......................................................................
 * Function to disconnect from the backend
 */
void Backend::disconnect()
{
  // first we check if we're connected
  if(connected_){
    // put try catch here
    cbassBackend_.backendDisconnect();
  };

  return;
};


/**.......................................................................
 * Function to issue commands to the backend
 */
void Backend::issueCommand(gcp::util::CbassBackend::Command command)
{
  int retVal;
  if(connected_){
    retVal = cbassBackend_.issueCommand(command);
    checkValid(retVal);
  }

  // do some error management.
  return;
}
void Backend::issueCommand(gcp::util::CbassBackend::Command command, unsigned char address)
{
  int retVal;
  if(connected_){
    retVal = cbassBackend_.issueCommand(command, address);
    checkValid(retVal);
  }

  // do some error management.
  return;
}
void Backend::issueCommand(gcp::util::CbassBackend::Command command, unsigned char* period)
{
  int retVal;
  if(connected_){
    retVal = cbassBackend_.issueCommand(command, period);
    checkValid(retVal);
  }

  // do some error management.
  return;
}
void Backend::issueCommand(gcp::util::CbassBackend::Command command, unsigned char address, unsigned char* period)
{
  int retVal;
  if(connected_){
    retVal = cbassBackend_.issueCommand(command, address, period);
    checkValid(retVal);
  }

  // do some error management.
  return;
}
void Backend::issueCommand(gcp::util::CbassBackend::Command command, unsigned char address, unsigned char* period, unsigned char channel, unsigned char stage)
{
  int retVal;
  if(connected_){
    retVal = cbassBackend_.issueCommand(command, address, period, channel, stage);
    checkValid(retVal);
  }

  // do some error management.
  return;
}


/**.......................................................................
 * Get data and write it to a buffer
 */
void Backend::getData()
{
  // I'm thinking I should fill in the values sequentially, and then
  // write them all to disk just once.
  int i,j;
  float thisTime;

  // check that we're connected.
  if(!connected_){
    return;
  };

  // first we call the getData method from CbassBackend;
  //  CTOUT("in backend.cc: got request for data.");
  int numBytes = cbassBackend_.getData();

  /// reset the index
  cbassBackend_.startIndex_ = 0;

  if(numBytes == 512){
    
    //  cbassBackend_.parseData(numBytes);
    cbassBackend_.parseData2011(numBytes);
    
    int numFrames = cbassBackend_.numFrames_;
    
    // now we write the frames to our buffer.
    for (i=0;i<numFrames;i++) {
      
      // first the time
      thisTime = cbassBackend_.sortTimeVals_[i];
      timeBuffer_[currentIndex_] = thisTime;
      
      // next the flags
      flagBuffer_[currentIndex_] = cbassBackend_.sortFlags_[i];
      
      // next the diagnostic data
      for (j=0;j<NUM_RECEIVER_DIAGNOSTICS;j++){
	diagnosticsBuffer_[currentIndex_][j] = cbassBackend_.sortDiagnostics_[i][j];
      };
      
      // next the data itself.
      for (j=0;j<NUM_RECEIVER_SWITCH_CHANNELS;j++){
      dataBuffer_[currentIndex_][j] = cbassBackend_.sortDataVals_[i][j];
      };
      
      // next the backend version
      versionBuffer_[currentIndex_] = cbassBackend_.sortBackendVersion_[i];
      
      // avg second length
      avgSecBuffer_[currentIndex_] = cbassBackend_.sortAvgSec_[i];
      
      
      // regular data buffer
      for(j=0;j<NUM_RECEIVER_CHANNELS;j++){
	regDataBuffer_[currentIndex_][j] = cbassBackend_.sortRegData_[i][j];
      };
      
      // alpha correction buffer 
      // should only have the last value set to anything.
      for (j=0;j<4;j++){
	alphaBuffer_[currentIndex_][j] = cbassBackend_.sortAlpha_[numFrames-1][j];
      }
      
      // nonlin correction buffer
      // should only have the last value set to anything.
      for (j=0;j<NUM_RECEIVER_SWITCH_CHANNELS;j++){
	nonlinBuffer_[currentIndex_][j] = cbassBackend_.sortNonlin_[numFrames-1][j];
      }
      
      
      // check that we're on to a new second
      if(thisTime < prevTime_) {
	prevSecEnd_   = currentIndex_ - 1;
	if(prevSecEnd_ < 0)
	  prevSecEnd_ = (MAX_BUFFER_LENGTH-1);  // it's an index, starting from zero.
	prevSecStart_ = thisSecStart_;
	thisSecStart_ = currentIndex_;
      };
      // set the previous time
      prevTime_ = thisTime;
      
      // increment the currentIndex;
      currentIndex_++;
      if(currentIndex_ == MAX_BUFFER_LENGTH) {
	currentIndex_ = 0;
      };
    };

  } else {
    if(numBytes < 0){
      ReportSimpleError("Backend returned -1 bytes.   Ignoring it for this frame");
      missedCommCounter_++;
      COUT("Missed Comm Counter Backend.cc"<<missedCommCounter_);
    } else {
      CTOUT("in backend.cc: got " << numBytes << "  bytes");
      ReportSimpleError("Backend returned wrong number of bytes.   Ignoring it for this frame");
      missedCommCounter_++;
      COUT("Missed Comm Counter Backend.cc"<<missedCommCounter_);
    }      
  };
  
  // all the data should be in the buffer now.    
  return;
}

/**.......................................................................
 * Get Burst data and write it to a file
 */
void Backend::getBurstData()
{
  int i,j, k;
  int index = 1;
  char thisLine[130];
  unsigned result;
  bool stop = false;
  char poo[2];
  int lineIndex = 0;

  
  // check that we're connected.
  if(!connected_){
    return;
  };

  // next we open a file to write things to.
  ofstream outputdata;

  // tie the file name to the utc.
  LogFile  logFile;
  logFile.setDirectory("/opt/data/cbass/burst/");
  logFile.setDatePrefix();

  std::string file = logFile.newFileName();
  
  outputdata.open(file.c_str());
  if(!outputdata.is_open()) {
    // could not open file
    COUT("Error: File could not be opened");
    return;
  };

  // if the file is open, we start our loop to read in the data.
  // we need to do this in a loop until there's no more data left to
  // read
  while(!stop) {
    
    // first we call the getData method from CbassBackend;
    int numBytes = cbassBackend_.getData();
    
    if(numBytes <= 0){
      // stop the loop.  we have no more data.
      stop = true;
    } else {

      // we need to split each line, and then write it to the file.
      // code lifted from parseData.
      for (j = 0; j < numBytes / 16; j++) {
	index = 1;
	for (i = 0; i < 16; i+=2) {
	  // turn the response into a string.
	  for(k=7;k >= 0;k--) {
	    result = cbassBackend_.data_[16*j+i+1] & (1U << k);
	    if(index==1){
	      sprintf(thisLine, "%d", result ? 1 : 0);
	      index = 0;
	    } else {
	      sprintf(poo, "%d", result ? 1 : 0);
	      strcat(thisLine, poo);
	    }
	  }
	  for(k=7;k >= 0;k--) {
	    result = cbassBackend_.data_[16*j+i] & (1U << k);
	    sprintf(poo, "%d", result ? 1 : 0);
	    strcat(thisLine, poo);
	  };
	};
	// thisLine is our line of 0s and 1s.  write this to the file
	outputdata << thisLine << endl;
	lineIndex++;

	/*
	  if(lineIndex>33000){
	  COUT("TOO MUCH DATA COMING BACK IN BURST MODE -- EXITING");
	  stop = true;
	};
	*/
      };
    };
  };
      
  // all the data should be in the file now, so we can close it.
  outputdata.close();

  return;
}


/**.......................................................................
 * Write the data to disk
 */
void Backend::writeData(gcp::util::TimeVal& currTime)
{
  int i,j, index;
  float thisMeanVal;
  float lo1, lo2, hi1, hi2;
  int numSamples = prevSecEnd_ - prevSecStart_ + 1;
  if(numSamples<0){
    numSamples += MAX_BUFFER_LENGTH;
  };
  gcp::util::TimeVal dataTime;

  if(!share_){
    // nothing to do if no share object
    return;
  }

#if 1
  if(numSamples > 100) {
    ReportError("Backend sent numSamples = " << numSamples);
    return;
  }
#endif

  // now all we have to do is write the data registers for the utc and
  // the actual values from prevSecStart to prevSecStop;
  static float dataVals[NUM_RECEIVER_CHANNELS][RECEIVER_SAMPLES_PER_FRAME];
  static float dataVector[NUM_RECEIVER_CHANNELS*RECEIVER_SAMPLES_PER_FRAME];
  static float switchDataVals[NUM_RECEIVER_SWITCH_CHANNELS][RECEIVER_SAMPLES_PER_FRAME];
  static float switchDataVector[NUM_RECEIVER_SWITCH_CHANNELS*RECEIVER_SAMPLES_PER_FRAME];
  static float diagnosticsVals[NUM_RECEIVER_DIAGNOSTICS][RECEIVER_SAMPLES_PER_FRAME];
  static float diagnosticsVector[NUM_RECEIVER_DIAGNOSTICS*RECEIVER_SAMPLES_PER_FRAME];


  static unsigned short flagVector[RECEIVER_SAMPLES_PER_FRAME];
  static std::vector<RegDate::Data> receiverUtc(RECEIVER_SAMPLES_PER_FRAME);
  

  // we're writing data from the previous second
  currTime.incrementSeconds(-1);
  // the curret message for when to write the data is not at the
  // second interval, though, it's 0.3 seconds after the 1PPS
  currTime.incrementSeconds(-0.3);

  index = prevSecStart_;
  for (i=0;i<numSamples;i++){
    dataTime = currTime;
    for (j=0;j<NUM_RECEIVER_SWITCH_CHANNELS;j++){
      switchDataVals[j][i] = dataBuffer_[index][j];
    };

    // calculate the old output from the backend.
    for (j=0;j<NUM_RECEIVER_CHANNELS;j++){
      lo1 = dataBuffer_[index][4*j+0];
      lo2 = dataBuffer_[index][4*j+2];
      hi1 = dataBuffer_[index][4*j+1];
      hi2 = dataBuffer_[index][4*j+3];
      thisMeanVal = (lo1-lo2+hi2-hi1)/2;
      dataVals[j][i] = thisMeanVal;
    };    

    // stuff for the diagnostics
    for (j=0;j<NUM_RECEIVER_DIAGNOSTICS;j++){
      diagnosticsVals[j][i] = diagnosticsBuffer_[index][j];
    }

    // timeBuffer_ ranges from 0 to 1 (it's a fraction of a secon)
    // we need to add it to the mjd from the previous second (in milliseconds) 
    dataTime.incrementSeconds(timeBuffer_[index]);
    receiverUtc[i] = dataTime;

    // add the flags as well.
    flagVector[i] = flagBuffer_[index];
    
    index++;
    if(index >= MAX_BUFFER_LENGTH){
      index = 0;
    }
  };

  // re-order the data registers into one long vector
  index = 0;
  for (i=0;i<NUM_RECEIVER_CHANNELS;i++){
    for (j=0;j<RECEIVER_SAMPLES_PER_FRAME;j++){
      if(j<numSamples){
	dataVector[index] = dataVals[i][j];
      } else {
	dataVector[index] = 0;
      };
      // increment the index.
      index++;
    };
  };
  // same for the switched data values.
  index = 0;
  for (i=0;i<NUM_RECEIVER_SWITCH_CHANNELS;i++){
    for (j=0;j<RECEIVER_SAMPLES_PER_FRAME;j++){
      if(j<numSamples){
	switchDataVector[index] = switchDataVals[i][j];
      } else {
	switchDataVector[index] = 0;
      };
      // increment the index.
      index++;
    };
  };

  // same for diagnostics
  index = 0;
  for (i=0;i<NUM_RECEIVER_DIAGNOSTICS;i++){
    for (j=0;j<RECEIVER_SAMPLES_PER_FRAME;j++){
      if(j<numSamples){
	diagnosticsVector[index] = diagnosticsVals[i][j];
      } else {
	diagnosticsVector[index] = 0;
      };
      // increment the index.
      index++;
    };
  };

  // set the times to sensible things and set the flag to high.
  if(numSamples<RECEIVER_SAMPLES_PER_FRAME){
    // set the other values in time to something.
    for (i=numSamples;i<RECEIVER_SAMPLES_PER_FRAME;i++){
      dataTime.incrementSeconds(MS_PER_RECEIVER_SAMPLE/10000);
      receiverUtc[i] = dataTime;
      flagVector[i] = (unsigned short) pow(2.0,14);
    };
  };


  // write the data registers
  share_->writeReg(rxSwitchData_, &switchDataVector[0]);

  // write the co-added data registers
  share_->writeReg(rxData_, &dataVector[0]);

  // write the time register
  share_->writeReg(rxUtc_, &receiverUtc[0]);

  // write the flag register
  share_->writeReg(rxFlags_, &flagVector[0]);

  // write the diagnostics
  share_->writeReg(rxDiagnostics_, &diagnosticsVector[0]);

  // that should do it.
  return;
}


/**.......................................................................
 * Write the data to disk
 */
void Backend::writeData2011(gcp::util::TimeVal& currTime)
{
  int i,j, index;
  float thisMeanVal;
  float thisAvg;
  int numSamples = prevSecEnd_ - prevSecStart_ + 1;
  if(numSamples<0){
    numSamples += MAX_BUFFER_LENGTH;
  };
  gcp::util::TimeVal dataTime;

  if(!share_){
    // nothing to do if no share object
    return;
  }

#if 1
  if(numSamples > 100) {
    ReportError("Backend sent numSamples = " << numSamples);
    return;
  }
#endif

  // now all we have to do is write the data registers for the utc and
  // the actual values from prevSecStart to prevSecStop;
  static float dataVals[NUM_RECEIVER_CHANNELS][RECEIVER_SAMPLES_PER_FRAME];
  static float dataVector[NUM_RECEIVER_CHANNELS*RECEIVER_SAMPLES_PER_FRAME];
  static float switchDataVals[NUM_RECEIVER_SWITCH_CHANNELS][RECEIVER_SAMPLES_PER_FRAME];
  static float switchDataVector[NUM_RECEIVER_SWITCH_CHANNELS*RECEIVER_SAMPLES_PER_FRAME];
  static float diagnosticsVals[NUM_RECEIVER_DIAGNOSTICS][RECEIVER_SAMPLES_PER_FRAME];
  static float diagnosticsVector[NUM_RECEIVER_DIAGNOSTICS*RECEIVER_SAMPLES_PER_FRAME];
  static float alphaVals[NUM_ALPHA_CORRECTIONS][RECEIVER_SAMPLES_PER_FRAME];
  static float alphaTotal[NUM_ALPHA_CORRECTIONS];
  static float alphaVector[NUM_ALPHA_CORRECTIONS];
  static float nonlinVals[NUM_RECEIVER_SWITCH_CHANNELS][RECEIVER_SAMPLES_PER_FRAME];
  static float nonlinVector[NUM_RECEIVER_SWITCH_CHANNELS];
  static float nonlinTotal[NUM_RECEIVER_SWITCH_CHANNELS];



  static unsigned short flagVector[RECEIVER_SAMPLES_PER_FRAME];
  static std::vector<RegDate::Data> receiverUtc(RECEIVER_SAMPLES_PER_FRAME);
  static float versionVector[RECEIVER_SAMPLES_PER_FRAME];
  static uint avgSecVector[RECEIVER_SAMPLES_PER_FRAME];

  // zero the sum values
  for (j=0;j<NUM_RECEIVER_SWITCH_CHANNELS;j++){
    nonlinTotal[j] = 0;
  }
  for (j=0;j<NUM_ALPHA_CORRECTIONS;j++){
    alphaTotal[j] = 0;
  };
  
  // we're writing data from the previous second
  currTime.incrementSeconds(-1);
  // the curret message for when to write the data is not at the
  // second interval, though, it's 0.3 seconds after the 1PPS
  currTime.incrementSeconds(-0.3);

  index = prevSecStart_;

  for (i=0;i<numSamples;i++){
    dataTime = currTime;
    for (j=0;j<NUM_RECEIVER_SWITCH_CHANNELS;j++){
      switchDataVals[j][i] = dataBuffer_[index][j];
    };


    // record "regular" data from new mode.
    for (j=0;j<NUM_RECEIVER_CHANNELS;j++){
      dataVals[j][i] = regDataBuffer_[index][j];
    };    


    // stuff for the diagnostics
    for (j=0;j<NUM_RECEIVER_DIAGNOSTICS;j++){
      diagnosticsVals[j][i] = diagnosticsBuffer_[index][j];
    }

    // timeBuffer_ ranges from 0 to 1 (it's a fraction of a secon)
    // we need to add it to the mjd from the previous second (in milliseconds) 
    dataTime.incrementSeconds(timeBuffer_[index]);
    receiverUtc[i] = dataTime;

    // add the flags as well.
    flagVector[i] = flagBuffer_[index];

    // for the coefficients, we're going to calculate a mean and see
    // if it's equal to the original value
    for (j=0;j<NUM_RECEIVER_SWITCH_CHANNELS; j++){
      nonlinTotal[j] += nonlinBuffer_[index][j];
    };

    for (j=0;j<NUM_ALPHA_CORRECTIONS;j++){
      alphaTotal[j] += alphaBuffer_[index][j];
    };

    // version vector
    versionVector[i] = versionBuffer_[index];

    // average second
    avgSecVector[i] = avgSecBuffer_[index];


    index++;
    if(index >= MAX_BUFFER_LENGTH){
      index = 0;
    }
  };

  // populate our nonlinVector and alphaVector
  // ultimately, we don't care if it changes mid-stream.
  for (j=0;j<NUM_RECEIVER_SWITCH_CHANNELS; j++){
    thisAvg = nonlinTotal[j]/(float) numSamples;
    nonlinVector[j] = thisAvg;
    //    thisAvg = thisAvg - nonlinBuffer_[0][j];
    //    thisAvg = abs(thisAvg);
    //if( thisAvg < acceptDelta) {
    //  nonlinVector[i] = thisAvg;
    //} else {
    //  nonlinVector[i] = 0;
    //};
  };

  for (j=0;j<NUM_ALPHA_CORRECTIONS; j++){
    thisAvg = alphaTotal[j]/(float) numSamples;
    alphaVector[j] = thisAvg;
  };


  // re-order the data registers into one long vector
  index = 0;
  for (i=0;i<NUM_RECEIVER_CHANNELS;i++){
    for (j=0;j<RECEIVER_SAMPLES_PER_FRAME;j++){
      if(j<numSamples){
	dataVector[index] = dataVals[i][j];
      } else {
	dataVector[index] = 0;
      };
      // increment the index.
      index++;
    };
  };
  // same for the switched data values.
  index = 0;
  for (i=0;i<NUM_RECEIVER_SWITCH_CHANNELS;i++){
    for (j=0;j<RECEIVER_SAMPLES_PER_FRAME;j++){
      if(j<numSamples){
	switchDataVector[index] = switchDataVals[i][j];
      } else {
	switchDataVector[index] = 0;
      };
      // increment the index.
      index++;
    };
  };

  // same for diagnostics
  index = 0;
  for (i=0;i<NUM_RECEIVER_DIAGNOSTICS;i++){
    for (j=0;j<RECEIVER_SAMPLES_PER_FRAME;j++){
      if(j<numSamples){
	diagnosticsVector[index] = diagnosticsVals[i][j];
      } else {
	diagnosticsVector[index] = 0;
      };
      // increment the index.
      index++;
    };
  };

  // set the times to sensible things and set the flag to high.
  if(numSamples<RECEIVER_SAMPLES_PER_FRAME){
    // set the other values in time to something.
    for (i=numSamples;i<RECEIVER_SAMPLES_PER_FRAME;i++){
      dataTime.incrementSeconds(MS_PER_RECEIVER_SAMPLE/10000);
      receiverUtc[i] = dataTime;
      flagVector[i] = (unsigned short) pow(2.0,14);
    };
  };


  // write the data registers
  share_->writeReg(rxSwitchData_, &switchDataVector[0]);

  // write the time register
  share_->writeReg(rxUtc_, &receiverUtc[0]);

  // write the flag register
  share_->writeReg(rxFlags_, &flagVector[0]);

  // write the diagnostics
  share_->writeReg(rxDiagnostics_, &diagnosticsVector[0]);

  // write the co-added data registers
  share_->writeReg(rxData_, &dataVector[0]);

  // write the average second
  share_->writeReg(rxSecLength_, &avgSecVector[0]);

  // write the diagnostics
  share_->writeReg(rxVersion_, &versionVector[0]);
  
  // write the alpha corrections
  share_->writeReg(rxAlpha_, &alphaVector[0]);
  
  // write the nonlinearity corrections
  share_->writeReg(rxNonlin_, &nonlinVector[0]);

  // that should do it.

  // reset the missedCommunication counter;
  missedCommCounter_ = 0;

  return;
}


/**.......................................................................
 * Check if we're connected
 */
bool Backend::isConnected()
{
  return connected_;
}

/**.......................................................................
 * Checks if response is valid.
 */
bool Backend::checkValid(int retVal)
{
  
  if(retVal>=0){
    isValid_ = true;
  } else {
    isValid_ = false;
  };
  
  return isValid_;
};
