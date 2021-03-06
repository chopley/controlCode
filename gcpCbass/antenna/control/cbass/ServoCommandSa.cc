#include <netinet/in.h> // Needed for htons()

#include "gcp/antenna/control/specific/ServoCommandSa.h"

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
ServoCommandSa::ServoCommandSa()
{
  request_ = INVALID;
  constructMap();
}

/**.......................................................................
 * Constructor initializes request type to invalid.
 */
ServoCommandSa::~ServoCommandSa() {};

/**.......................................................................
 * pack all our commands
 */
void ServoCommandSa::packCommand(Request req)
{
  std::vector<float> values;
  return packCommand(req, values);
};

void ServoCommandSa::packCommand(Request req, std::vector<float>& values)
{
  request_ = req;
  switch(request_) {

    /* SERVO COMMANDS */
    /* no \r in south africa */

    /* Commands that take no inputs, echo the output */
  case AZ_BRAKE_ON:     
  case AZ_BRAKE_OFF:    
  case EL_BRAKE_ON:     
  case EL_BRAKE_OFF:    
  case CLUTCHES_ON:     
  case CLUTCHES_OFF:    
  case AZ_CONTACTORS_ON:
  case AZ_CONTACTORS_OFF:
  case EL_CONTACTORS_ON: 
  case EL_CONTACTORS_OFF:
    {
      std::ostringstream os;
      os << commandMap_[req];
      messageToSend_ = os.str();
      expectedResponse_ = commandMap_[req];
    }
    break;

    /* Take one input, echo the output */
  case SERVO_ENGAGE:
    {
      std::ostringstream os;
      os << commandMap_[req] << "," << setprecision(1) << setw(1) << values[0];
      messageToSend_ = os.str();
      expectedResponse_ = commandMap_[req];
    }
    break;

    /* These take two inputs, and echo the output */
  case SEND_POS:       
    {
      
      if(values.size() != 2) {
	/* wrong number of inputs */
	throw Error("ServoCommandSa::packCommand: wrong number of inputs \n");
      };
      std::ostringstream os;
      os << commandMap_[req] << "," << setprecision(7) << setw(7) << values[0] << "," << values[1];
      messageToSend_ = os.str();
      //      COUT("message: " << messageToSend_);
      expectedResponse_ = commandMap_[req];
    }
    break;

    /* These take eight inputs, and echo the output */
  case SEND_POS_TRIP:       
    {
      
      if(values.size() != 8) {
	/* wrong number of inputs */
	throw Error("ServoCommandSa::packCommand: wrong number of inputs \n");
      };
      int i;
      std::ostringstream os;
      os << commandMap_[req];
      for (i=0;i<8;i++){
	switch(i){
	case 6:
	case 7:
	  // time values
	  os << "," << setprecision(5) << setw(5) << values[i];
	  break;
	  
	default:
	  os << "," << setprecision(7) << setw(7) << values[i];
	  break;
	}
      }
      messageToSend_ = os.str();
      //      COUT("request: " << messageToSend_);
      expectedResponse_ = commandMap_[req];
    }
    break;

    /* Many inputs, echo output */
  case LOAD_LOOP_PARAMS_A:
  case LOAD_LOOP_PARAMS_B:
  case LOAD_LOOP_PARAMS_C:
  case LOAD_LOOP_PARAMS_D:
    {
      if(values.size() != 7) {
	/* wrong number of inputs */
	throw Error("ServoCommandSa::packCommand: wrong number of inputs \n");
      };
      int i;
      std::ostringstream os;
      os << commandMap_[req];
      for (i=0;i<7; i++){
	switch(i) {
	case 0:
	  os <<  "," << setprecision(6) << setw(5) ;
	  break;

	case 1:
	case 2:
	  os <<  "," << setprecision(4) << setw(3) ;
	  break;

	case 3:
	case 4:
	  os <<  "," << setprecision(2) << setw(2) ;
	  break;

	case 5:
	  os <<  "," << setprecision(2) << setw(2) ;
	  break;

	case 6:
	  os <<  "," << setprecision(4) << setw(4) ;
	  break;

	default:
	  os <<  "," << setprecision(2) << setw(2) ;
	  break;
	  
	};
	/* add the value */
	os << values[i];
      };
      messageToSend_ = os.str();
      //      COUT(" command:  " << messageToSend_);
      expectedResponse_ = commandMap_[req];
    };
    //   COUT("MESSAGE: " << messageToSend_);
    break;
    
    /* No Inputs, 1 Output */
  case QUERY_STATUS: 
    {
      std::ostringstream os;
      os << commandMap_[req];
      messageToSend_ = os.str();
      std::ostringstream os2;
      os2 << commandMap_[req] << ",";
      expectedResponse_ = os2.str();
    }    
    break;

    /* No Inputs, 2 Outputs */
  case GET_AZEL:        
  case POSITION_ERRORS:
    {
      std::ostringstream os;
      os << commandMap_[req]; 
      messageToSend_ = os.str();
      std::ostringstream os2;
      os2 << commandMap_[req] << ",";
      expectedResponse_ = os2.str();
    }    
    break;

    /* No Inputs, Many outputs */
  case GET_PRIOR_LOC:
    {
      std::ostringstream os;
      os << commandMap_[req];
      messageToSend_ = os.str();
      std::ostringstream os2;
      os2 << commandMap_[req] << ",";
      expectedResponse_ = os2.str();
    }    
    break;
  };

  cmdSize_ = messageToSend_.size();
  expectsResponse_ = true;
  responseLength_ = expectedResponse_.size();
  return;
};



/**.......................................................................
 * Return our size as a size_t suitable for passing to write(2) or
 * read(2).
 */
size_t ServoCommandSa::size()
{
  return static_cast<size_t>(cmdSize_);
}

/**.......................................................................
 * Return the response length.
 */
size_t ServoCommandSa::responseLength()
{
  return static_cast<size_t>(responseLength_);
}

/**.......................................................................
 * Interpret Command to see if something valid happened.
 */
void ServoCommandSa::interpretResponse()
{
  int i,len;

  switch(request_) { 
    /* SERVO COMMANDS */
    /* Commands that echo on success -- no values to report */
  case SERVO_ENGAGE:
  case AZ_BRAKE_ON:     
  case AZ_BRAKE_OFF:    
  case EL_BRAKE_ON:     
  case EL_BRAKE_OFF:    
  case CLUTCHES_ON:     
  case CLUTCHES_OFF:    
  case AZ_CONTACTORS_ON:
  case AZ_CONTACTORS_OFF:
  case EL_CONTACTORS_ON: 
  case EL_CONTACTORS_OFF:
  case SEND_POS:       
  case SEND_POS_TRIP:       
  case LOAD_LOOP_PARAMS_A:
  case LOAD_LOOP_PARAMS_B:
  case LOAD_LOOP_PARAMS_C:
  case LOAD_LOOP_PARAMS_D:
    responseValid_ = simpleValidityCheck();
    break;


    /* No Inputs, 1 Output */
  case QUERY_STATUS: 
    checkOneOutput(); 
    break;

    /* No Inputs, 2 Outputs */
  case GET_AZEL:        
  case POSITION_ERRORS:
    checkTwoOutputs();
    break;

    /* No Inputs, Many outputs */
  case GET_PRIOR_LOC:
    checkManyOutputs();
    break;
    
  default: 
    responseValid_ = simpleValidityCheck();
    break;
  };   
  
  return;
  
};

/**.......................................................................
 * Simple Check if two strings match
 */
bool ServoCommandSa::simpleValidityCheck()
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
void ServoCommandSa::checkOneOutput()
{
  std::string respStr(responseReceived_);
  String responseString(respStr);
  String expectedString(expectedResponse_);

  switch(request_){
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
void ServoCommandSa::interpretStatusResponse(const char* response)
{
  /* This status response takes form of 21 values
     The first 19 are ones,zeros, the last 2 go from 0 to 2.
  */

  int i;
  for (i=0; i<19; i++) {
    if( *(response + 4 + i) == '1') {
      responseValue_[i] = 1;
    } else {
      responseValue_[i] = 0;
    }
  };
  
  // last two
  for (i=19;i<21;i++) {
    if( *(response + 4 + i) == '0') {
      responseValue_[i] = 0;
    } else if( *(response + 4 + i) == '1') {
      responseValue_[i] = 1;
    } else if( *(response + 4 + i) == '2') {
      responseValue_[i] = 2;
    }
  };

  return;
};


/**.......................................................................
 * Check if box output is valid, and what its value is.
 */
void ServoCommandSa::checkTwoOutputs()
{
  std::string respStr(responseReceived_);
  String responseString(respStr);
  String expectedString(expectedResponse_);

  switch(request_){
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
void ServoCommandSa::checkManyOutputs()
{
  std::string respStr(responseReceived_);
  String responseString(respStr);
  String expectedString(expectedResponse_);
  float errorType;
  
  switch(request_){
  case GET_PRIOR_LOC:
    std::string badResponse("NODATA");
    String badResp(badResponse);
    int i;

    {
      String prefix = responseString.findNextStringSeparatedByChars(",");
      String prefixExp = expectedString.findNextStringSeparatedByChars(",");
     String prefixBad = badResp.findNextStringSeparatedByChars(",");
      // Test the return string:
      if(prefix.str() != prefixExp.str()){
	// check that we just don't have data for that second
	if(prefix.str() == prefixBad.str()) {
	  responseValid_ = true;
	  responseValueValid_ = false;
	  ReportMessage("NODATA returned from Servo");
	} else {
	  responseValueValid_ = false;
	  responseValid_ = false;
	}
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
void ServoCommandSa::constructMap()
{
  /* First set is for the Douloi box */
  commandMap_[INVALID]            = "";
  commandMap_[SERVO_ENGAGE]       = "SVE";  // one input, echo output
  commandMap_[GET_AZEL]           = "GAE";
  commandMap_[GET_PRIOR_LOC]      = "GIM";
  commandMap_[SEND_POS]           = "AEL";  //needs further input
  commandMap_[SEND_POS_TRIP]      = "AEL";  //needs further input
  commandMap_[LOAD_LOOP_PARAMS_A] = "LPA";  //needs further input
  commandMap_[LOAD_LOOP_PARAMS_B] = "LPB";  //needs further input
  commandMap_[LOAD_LOOP_PARAMS_C] = "LPC";  //needs further input
  commandMap_[LOAD_LOOP_PARAMS_D] = "LPD";  //needs further input
  commandMap_[QUERY_STATUS]       = "STS";
  commandMap_[AZ_BRAKE_ON]        = "ABN";
  commandMap_[AZ_BRAKE_OFF]       = "ABF";
  commandMap_[EL_BRAKE_ON]        = "EBN";
  commandMap_[EL_BRAKE_OFF]       = "EBF";
  commandMap_[CLUTCHES_ON]        = "CLN";
  commandMap_[CLUTCHES_OFF]       = "CLF";
  commandMap_[AZ_CONTACTORS_ON]   = "AMN";
  commandMap_[AZ_CONTACTORS_OFF]  = "AMF";
  commandMap_[EL_CONTACTORS_ON]   = "EMN";
  commandMap_[EL_CONTACTORS_OFF]  = "EMF";
  commandMap_[POSITION_ERRORS]    = "ERR";

 };



