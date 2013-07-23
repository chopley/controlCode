#include <netinet/in.h> // Needed for htons()

#include "gcp/antenna/control/specific/ServoCommand.h"

#include "gcp/util/common/DataArray.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/String.h"
#include <sys/ioctl.h>
#include <string>
#include <map>
#include <iomanip>
#include <sstream>

#include <string.h>

using namespace std;
using namespace gcp::antenna::control;
using namespace gcp::util;

/**.......................................................................
 * Constructor initializes request type to invalid and constructs the command map.
 */
ServoCommand::ServoCommand()
{
  request_ = INVALID;
  constructMap();
}

/**.......................................................................
 * Constructor initializes request type to invalid.
 */
ServoCommand::~ServoCommand() {};

/**.......................................................................
 * pack all our commands
 */
void ServoCommand::packCommand(Request req)
{
  std::vector<float> values;
  return packCommand(req, values);
};

void ServoCommand::packCommand(Request req, std::vector<float>& values)
{
  request_ = req;
  switch(request_) {

    /* SERVO COMMANDS */
    /* Commands that take no inputs, echo the output */
  case BEGIN_PPS_LOOP:  
  case BEGIN_TRACK_LOOP:
  case STOP_ALL:        
  case CAL_ENCODERS:    
  case CLEAR_OFFSETS:   
  case AZ_BRAKE_ON:     
  case AZ_BRAKE_OFF:    
  case EL_BRAKE_ON:     
  case EL_BRAKE_OFF:    
    {
      std::ostringstream os;
      os << commandMap_[req] << "\r";
      messageToSend_ = os.str();
      expectedResponse_ = commandMap_[req];
    }
    break;

    /* These take two inputs, and echo the output */
  case SEND_POS:       
  case POSITION_OFFSET:
  case VELOCITY_OFFSET:
  case SET_AZ_TORQUES: 
  case SET_ENCODERS:   
    /* tested most of them, they work */
    {
      
      if(values.size() != 2) {
	/* wrong number of inputs */
	throw Error("ServoCommand::packCommand: wrong number of inputs \n");
      };
      std::ostringstream os;
      os << commandMap_[req] << "," << setprecision(7) << setw(7) << values[0] << "," << values[1] << '\r';
      messageToSend_ = os.str();
      expectedResponse_ = commandMap_[req];
    }
    break;

    /* Take one input, echos the output */
  case SET_AZ_TORQUE:
  case SET_EL_TORQUE:
    {
      if(values.size() != 1) {
	/* wrong number of inputs */
	throw Error("ServoCommand::packCommand: wrong number of inputs \n");
      };
    std::ostringstream os;
    os << commandMap_[req] << "," << setprecision(3) << setw(6) << values[0] << '\r';
    messageToSend_ = os.str();
    expectedResponse_ = commandMap_[req];
    }
    break;    

    /* Many inputs, echo output */
  case LOAD_LOOP_PARAMS:
    {
      if(values.size() != 37) {
	/* wrong number of inputs */
	throw Error("ServoCommand::packCommand: wrong number of inputs \n");
      };
      int i;
      std::ostringstream os;
      os << commandMap_[req];
      for (i=0;i<37; i++){
	switch(i) {
	case 4:
	  os <<  "," << setprecision(2) << setw(3) ;
	  break;

	case 16:
	case 17:
	  os <<  "," << setprecision(2) << setw(2) ;
	  break;
	  
	case 22:
	  os <<  "," << setprecision(3) << setw(3) ;
	  break;
	case 23:
	  os <<  "," << setprecision(4) << setw(4) ;
	  break;

	case 24:
	  os <<  "," << setprecision(3) << setw(4) ;
	  break;
	case 25:
	  os <<  "," << setprecision(4) << setw(5) ;
	  break;

	case 28:
	  os <<  "," << setprecision(3) << setw(3) ;
	  break;

	case 29:
	case 30:
	  os <<  "," << setprecision(6) << setw(6) ;
	  break;

	case 31:
	case 32:
	  os <<  "," << setprecision(5) << setw(5) ;
	  break;

	case 33:
	case 34:
	  os <<  "," << setprecision(6) << setw(7) ;
	  break;

	case 35:
	  os <<  "," << setprecision(4) << setw(4) ;
	  break;
	case 36:
	  os <<  "," << setprecision(5) << setw(5) ;
	  break;

	default:
	  os <<  "," << setprecision(2) << setw(2) ;
	  break;
	  
	};
	/* add the value */
	os << values[i];
      };
      /* append carriage return to end */
      os << "\r";
      messageToSend_ = os.str();
      // COUT(" command:  " << messageToSend_);
      expectedResponse_ = commandMap_[req];
    };
    //   COUT("MESSAGE: " << messageToSend_);
    break;
    
    /* No Inputs, Expected Output */
  case QUERY_VERSION:
    {
      std::ostringstream os;
      os << commandMap_[req] << "\r";
      messageToSend_ = os.str();
      expectedResponse_ = "Ver 6mCBASS4";
    }
    COUT(" VERSION RESPONSE:  " << expectedResponse_);
    break;

    /* No Inputs, 1 Output */
  case NUMBER_ERRORS:
  case GET_PPS_TICK: 
  case QUERY_STATUS: 
    {
      std::ostringstream os;
      os << commandMap_[req] << "\r";
      messageToSend_ = os.str();
      std::ostringstream os2;
      os2 << commandMap_[req] << ",";
      expectedResponse_ = os2.str();
    }    
    break;

    /* No Inputs, 2 Outputs */
  case READ_AZ_ENCODERS:
  case POSITION_ERRORS: 
    {
      std::ostringstream os;
      os << commandMap_[req] << "\r";
      messageToSend_ = os.str();
      std::ostringstream os2;
      os2 << commandMap_[req] << ",";
      expectedResponse_ = os2.str();
    }    
    break;
  case GET_AZEL:        
    {
      std::ostringstream os;
      os << commandMap_[req] << "\r";
      messageToSend_ = os.str();
      std::ostringstream os2;
      // response different than request
      os2 << "AZEL,";
      expectedResponse_ = os2.str();
    }    
    break;

    /* No Inputs, Many outputs */
  case GET_PRIOR_LOC:
    {
      std::ostringstream os;
      os << commandMap_[req] << "\r";
      messageToSend_ = os.str();
      std::ostringstream os2;
      os2 << commandMap_[req] << ",";
      expectedResponse_ = os2.str();
    }    
    break;
    
    /*  AMPLIFIER COMMANDS */
    /* Commands that expect 2A0#> response */
  case START_AMP_COMMS:
  case ENABLE_AZ_AMP1: 
  case ENABLE_AZ_AMP2: 
  case ENABLE_EL_AMP: 
  case DISABLE_AZ_AMP1:
  case DISABLE_AZ_AMP2:
  case DISABLE_EL_AMP: 
  case ALIVE_AZ_AMP1: 
  case ALIVE_AZ_AMP2: 
  case ALIVE_EL_AMP:  
    {
      messageToSend_ = commandMap_[req];
      int i;
      std::ostringstream os;
      for (i=0; i<4; i++){
	os << messageToSend_[i];
      };
      os << ">";
      expectedResponse_ = os.str();
    };
    break;
    
    /* Commands that expect 0/1 in response */
  case STATUS_AZ_AMP1:
  case STATUS_AZ_AMP2:
  case STATUS_EL_AMP:
    {
      messageToSend_ = commandMap_[req];
      int i;
      std::ostringstream os;
      for (i=0; i<4; i++){
	os << messageToSend_[i];
      };
      expectedResponse_ = os.str();
    };
    break;
    
    /* Commands that expect a value in response */
  case ACTUAL_CURR_AZ1:
  case ACTUAL_CURR_AZ2:
  case ACTUAL_CURR_EL: 
  case COMMAND_CURR_AZ1:
  case COMMAND_CURR_AZ2:
  case COMMAND_CURR_EL: 
    {
      messageToSend_ = commandMap_[req];
      int i;
      std::ostringstream os;
      for (i=0; i<4; i++){
	os << messageToSend_[i];
      };
      expectedResponse_ = os.str();
    };
    break;
  };
  cmdSize_ = messageToSend_.size();
  expectsResponse_ = true;
  responseLength_ = expectedResponse_.size();
  return;
};


/**.......................................................................
 * begin communications with the amplifier
 */
void ServoCommand::packStartAmpComms()
{
  request_ = START_AMP_COMMS;
  messageToSend_ = "2A01\r";
  expectedResponse_ = "2A01>";

  cmdSize_ = messageToSend_.size();

  expectsResponse_ = true;
  responseLength_  = expectedResponse_.size();
};

/**.......................................................................
 * enable the elevation amplifier
 */
void ServoCommand::packEnableElAmp()
{
  request_ = ENABLE_EL_AMP;
  messageToSend_ = "2A03EN1\r";
  expectedResponse_ = "2A03>";
  cmdSize_ = messageToSend_.size();

  expectsResponse_ = true;
  responseLength_  = expectedResponse_.size();
};

/**.......................................................................
 * disable the elevation amplifier
 */
void ServoCommand::packDisableElAmp()
{
  request_ = DISABLE_EL_AMP;
  messageToSend_ ="2A03EN0\r";
  expectedResponse_ ="2A03>";

  cmdSize_ = messageToSend_.size();
  expectsResponse_ = true;
  responseLength_  = expectedResponse_.size();
};

/**.......................................................................
 * enable the azimuth first amplifier
 */
void ServoCommand::packEnableAzAmp1()
{
  request_ = ENABLE_AZ_AMP1;
  messageToSend_ ="2A01EN1\r";
  expectedResponse_ ="2A01>";

  cmdSize_ = messageToSend_.size();
  expectsResponse_ = true;
  responseLength_  = expectedResponse_.size();
};

/**.......................................................................
 * disable the azimuth first amplifier
 */
void ServoCommand::packDisableAzAmp1()
{
  request_ = DISABLE_AZ_AMP1;
  messageToSend_ ="2A01EN0\r";
  expectedResponse_ ="2A01>";

  cmdSize_ = messageToSend_.size();

  expectsResponse_ = true;
  responseLength_  = expectedResponse_.size();
};


/**.......................................................................
 * Return our size as a size_t suitable for passing to write(2) or
 * read(2).
 */
size_t ServoCommand::size()
{
  return static_cast<size_t>(cmdSize_);
}

/**.......................................................................
 * Return the response length.
 */
size_t ServoCommand::responseLength()
{
  return static_cast<size_t>(responseLength_);
}

/**.......................................................................
 * Interpret Command to see if something valid happened.
 */
void ServoCommand::interpretResponse()
{
  int i,len;

  switch(request_) { 
    /* SERVO COMMANDS */
    /* Commands that echo on success -- no values to report */
  case BEGIN_PPS_LOOP:  
  case BEGIN_TRACK_LOOP:
  case STOP_ALL:        
  case CAL_ENCODERS:    
  case CLEAR_OFFSETS:   
  case AZ_BRAKE_ON:     
  case AZ_BRAKE_OFF:    
  case EL_BRAKE_ON:     
  case EL_BRAKE_OFF:    
  case SEND_POS:       
  case POSITION_OFFSET:
  case VELOCITY_OFFSET:
  case SET_AZ_TORQUES: 
  case SET_ENCODERS:   
  case SET_AZ_TORQUE:
  case SET_EL_TORQUE:
  case LOAD_LOOP_PARAMS:
    responseValid_ = simpleValidityCheck();
    break;

    /* No Inputs, Expected Output */
  case QUERY_VERSION:
    responseValid_ = simpleValidityCheck();
    break;

    /* No Inputs, 1 Output */
  case NUMBER_ERRORS:
  case GET_PPS_TICK: 
  case QUERY_STATUS: 
    checkOneOutput(); 
    break;

    /* No Inputs, 2 Outputs */
  case READ_AZ_ENCODERS:
  case GET_AZEL:        
  case POSITION_ERRORS: 
    checkTwoOutputs();
    break;

    /* No Inputs, Many outputs */
  case GET_PRIOR_LOC:
    checkManyOutputs();
    break;
    
    /*  AMPLIFIER COMMANDS */
  case START_AMP_COMMS:
    responseValid_ = simpleValidityCheck();
    if(responseValid_ == 0){
      /* in this case a simple check might not be valid because it's
	 the first command we issue */
      int len = strlen(responseReceived_);
      for (i=0; i<len; i++){
	if(*(responseReceived_+i)=='>'){
	  responseValid_ = true;
	};
      };
    };
    break;

    /* Commands that expect 2A0#> response */
  case ENABLE_AZ_AMP1: 
  case ENABLE_AZ_AMP2: 
  case ENABLE_EL_AMP: 
  case DISABLE_AZ_AMP1:
  case DISABLE_AZ_AMP2:
  case DISABLE_EL_AMP: 
  case ALIVE_AZ_AMP1: 
  case ALIVE_AZ_AMP2: 
  case ALIVE_EL_AMP:  
    responseValid_ = simpleValidityCheck();
    break;
    
    /* Commands that expect a value in response */
  case STATUS_AZ_AMP1:
  case STATUS_AZ_AMP2:
  case STATUS_EL_AMP:
  case ACTUAL_CURR_AZ1:
  case ACTUAL_CURR_AZ2:
  case ACTUAL_CURR_EL: 
  case COMMAND_CURR_AZ1:
  case COMMAND_CURR_AZ2:
  case COMMAND_CURR_EL:
    checkOneOutput();
    break;

  default: 
    responseValid_ = simpleValidityCheck();
    break;
  };   
  
    

  /* again, we need the switch statements to check the response value
     for each command   (responseValue_ */




  return;
  
};

/**.......................................................................
 * Simple Check if two strings match
 */
bool ServoCommand::simpleValidityCheck()
{
  string expResp (expectedResponse_);
  string recResp (responseReceived_);
  bool isValid;

  if(expResp.compare(recResp) == 0){
    isValid = true;
  } else { 
    isValid = false;
  };

  return isValid;
};


/**.......................................................................
 * Check if box output is valid, and what its value is.
 */
void ServoCommand::checkOneOutput()
{
  std::string respStr(responseReceived_);
  String responseString(respStr);
  String expectedString(expectedResponse_);

  switch(request_){
  case STATUS_AZ_AMP1:
  case STATUS_AZ_AMP2:
  case STATUS_EL_AMP:
  case ACTUAL_CURR_AZ1:
  case ACTUAL_CURR_AZ2:
  case ACTUAL_CURR_EL: 
  case COMMAND_CURR_AZ1:
  case COMMAND_CURR_AZ2:
  case COMMAND_CURR_EL:
    /* Tested, and they work */
    /* these will have 2A01+, where + is the value of the response */
    {
      /* check the length of our responses for the sake of turning them into char arrays */
      int lenResp = responseString.str().length();
      int lenComp = expectedString.str().length();
      const char* str1 = responseString.str().c_str();
      char str2[lenComp+1];
      int i;
      for (i=0;i<lenComp;i++){
	*(str2+i) = *(str1+i);
      };
      *(str2+lenComp) = '\0';  // Have to null terminate the new string or else it's gibberish.
      const char* str3 = expectedString.str().c_str();
      /* if the response is valid, str2 and str3 should be the same */
      if(strcmp((const char*) str2, str3)==0){
	/* response is valid, and get its value */
	responseValid_ = true;
	responseValue_[0] = atof((str1+lenComp));
      } else {
	responseValid_ = false;
      };
    }
    break;
    
  case NUMBER_ERRORS:
  case GET_PPS_TICK: 
    /* works great */
    {
      String prefix = responseString.findNextStringSeparatedByChars(", ");
      String prefixExp = expectedString.findNextStringSeparatedByChars(", ");
      
      // Test the return string:

      if(prefix.str() != prefixExp.str()){
	responseValid_ = false;
      } else {
	responseValid_ = true;
	responseValue_[0] = responseString.findNextStringSeparatedByChars(", ").toFloat();
      };
    };
    break;

  case QUERY_STATUS: 
    {
      String prefix = responseString.findNextStringSeparatedByChars(", ");
      String prefixExp =expectedString.findNextStringSeparatedByChars(", ");
      
      // Test the return string:

      if(prefix.str() != prefixExp.str()){
	responseValid_ = false;
      } else {
	responseValid_ = true;
	interpretStatusResponse(responseString.str().c_str());
      };
    };
    break;
  };
  return;
};


/**.......................................................................
 * Interpret our awful status response
 */
void ServoCommand::interpretStatusResponse(const char* response)
{
  /* This stupid status response takes form:  #BBBBBBBBBBBB#BB 
                                              3FFFFFFFFFFFF0TF
     There should be 16 values.  we'll translate these into 16 floats in the response */

  int i;
  for (i=0; i<16; i++) {
    switch(i){
    case 0:
    case 13:
      {
	responseValue_[i] = atoi((response + 4 + i));
      }
      break;
      
    default:
      {
	if( *(response + 4 + i) == 'T' ){
	  responseValue_[i] = 1;
	} else {
	  responseValue_[i] = 0;
	};
      };
      break;
    };
  };
};


/**.......................................................................
 * Check if box output is valid, and what its value is.
 */
void ServoCommand::checkTwoOutputs()
{
  std::string respStr(responseReceived_);
  String responseString(respStr);
  String expectedString(expectedResponse_);

  switch(request_){
  case READ_AZ_ENCODERS:
  case GET_AZEL:        
  case POSITION_ERRORS: 
    {
      String prefix = responseString.findNextStringSeparatedByChars(",");
      String prefixExp = expectedString.findNextStringSeparatedByChars(",");
      
      // Test the return string:
      
      if(prefix.str() != prefixExp.str()){
	responseValid_ = false;
      } else {
	responseValid_ = true;
	//	COUT("STRING TO CONVERT: " << responseString);
	responseValue_[0] = responseString.findNextStringSeparatedByChars(",").toFloat();
	responseValue_[1] = responseString.findNextStringSeparatedByChars(",").toFloat();
      };
    };
    break;
  };

  return;
};

/**.......................................................................
 * Check if box output is valid, and what its value is.
 */
void ServoCommand::checkManyOutputs()
{
  std::string respStr(responseReceived_);
  String responseString(respStr);
  String expectedString(expectedResponse_);
  float errorType;
  
  switch(request_){
  case GET_PRIOR_LOC:
    std::string badResponse("NOPPS,");
    String badResp(badResponse);
    int i;

    {
      String prefix = responseString.findNextStringSeparatedByChars(",");
      String prefixExp = expectedString.findNextStringSeparatedByChars(",");
      String prefixBad = badResp.findNextStringSeparatedByChars(",");
      // Test the return string:
      if(prefix.str() != prefixExp.str()){
	// check that we don't have a no 1PPS response
	if(prefix.str() == prefixBad.str()) {
	  // our response is valid, but there's no 1PPS
	  responseValid_ = true;
	  errorType = responseString.findNextStringSeparatedByChars(",").toFloat();
	  // errorType of 0 is a lack of 1PPS, errorType of 1 is wrong number of data points
	  if(errorType == 0){
	    COUT("1PPS not present");
	    responseValueValid_ = false;
	  } else if (errorType==1) {
	    ReportSimpleError("Number of data returned from Servo box is incorrect -- Timing issue");
	    COUT("message back: " << responseReceived_);
	    responseValueValid_ = false;
	  } else {
	    throw Error("ServoCommand::checkManyInputs: Response from Servo not recognized \n");
	  };
	} else { 
	  responseValueValid_ = false;
	  responseValid_ = false;
	};
      } else {
	responseValid_ = true;
	responseValueValid_ = true;
	for (i=0;i<MAX_RESPONSE_SAMPLES;i++){
	  responseValue_[i] = responseString.findNextStringSeparatedByChars(",").toFloat();
	  /* responses are in form:  az, el, erraz, errel */
	};
      };
    };
    break;
  };    

};


/**.......................................................................
 * Construct our map from enums to strings to issue.
 */
void ServoCommand::constructMap()
{
  /* First set is for the Douloi box */
  commandMap_[INVALID]          = "";
  commandMap_[GET_AZEL]         = "GAE";
  commandMap_[GET_PRIOR_LOC]    = "GIM";
  commandMap_[GET_PPS_TICK]     = "GPPS";
  commandMap_[SEND_POS]         = "AEL";  //needs further input
  commandMap_[BEGIN_PPS_LOOP]   = "TIM";
  commandMap_[BEGIN_TRACK_LOOP] = "TON";
  commandMap_[LOAD_LOOP_PARAMS] = "LPR"; //needs further input
  commandMap_[STOP_ALL]         = "SPA";
  commandMap_[CAL_ENCODERS]     = "CLE";
  commandMap_[CLEAR_OFFSETS]    = "CLO";
  commandMap_[POSITION_OFFSET]  = "POF"; //needs more input
  commandMap_[VELOCITY_OFFSET]  = "VOF"; //needs more input
  commandMap_[POSITION_ERRORS]  = "ERR";
  commandMap_[SET_ENCODERS]     = "SEN"; //needs more input
  commandMap_[QUERY_STATUS]     = "STS";
  commandMap_[AZ_BRAKE_ON]      = "ABN";
  commandMap_[AZ_BRAKE_OFF]     = "ABF";
  commandMap_[EL_BRAKE_ON]      = "EBN";
  commandMap_[EL_BRAKE_OFF]     = "EBF";
  commandMap_[SET_AZ_TORQUE]    = "AZV"; //needs more input
  commandMap_[SET_EL_TORQUE]    = "ELV"; //needs more input
  commandMap_[QUERY_VERSION]    = "VER";
  commandMap_[SET_AZ_TORQUES]   = "AZS"; //needs more input
  commandMap_[READ_AZ_ENCODERS] = "ENC";
  commandMap_[NUMBER_ERRORS]    = "NER";


  /* Second set is for the electronics card */
  /* 2A01 is el, 2A02 is az1, 2A03 is az2   */
  commandMap_[START_AMP_COMMS]  = "2A01";    // if not completed, no >
  commandMap_[ENABLE_AZ_AMP1]   = "2A02EN1"; // if not completed, no >
  commandMap_[ENABLE_AZ_AMP2]   = "2A03EN1";
  commandMap_[ENABLE_EL_AMP]    = "2A01EN1";
  commandMap_[DISABLE_AZ_AMP1]  = "2A02EN0"; //2A01> if not completed, no >
  commandMap_[DISABLE_AZ_AMP2]  = "2A03EN0";
  commandMap_[DISABLE_EL_AMP]   = "2A01EN0";
  commandMap_[STATUS_AZ_AMP1]   = "2A02EN";  //2A01(0-1) for disable/enable
  commandMap_[STATUS_AZ_AMP2]   = "2A03EN";
  commandMap_[STATUS_EL_AMP]    = "2A01EN";
  commandMap_[ALIVE_AZ_AMP1]    = "2A02RST"; //if 2A01>, the amp is communicating.
  commandMap_[ALIVE_AZ_AMP2]    = "2A03RST";
  commandMap_[ALIVE_EL_AMP]     = "2A01RST";
  commandMap_[ACTUAL_CURR_AZ1]  = "2A02SIA"; //2A01VAL (in abs term)
  commandMap_[ACTUAL_CURR_AZ2]  = "2A03SIA";
  commandMap_[ACTUAL_CURR_EL]   = "2A01SIA";
  commandMap_[COMMAND_CURR_AZ1] = "2A02SIC"; //2A01+-VAL
  commandMap_[COMMAND_CURR_AZ2] = "2A03SIC";
  commandMap_[COMMAND_CURR_EL]  = "2A01SIC";
};



