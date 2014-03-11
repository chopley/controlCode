#ifndef SERVOCOMMANDSA_H
#define SERVOCOMMANDSA_H

/**
 * @file ServoCommandSa.h
 * 
 * Tagged: Thu Nov 13 16:53:43 UTC 2003
 * 
 * @author Erik Leitch
 *  Modified for CBASS by Stephen Muchovej
 */
#include <string>
#include <map>
#include <vector>

// The hard limit -- pmac can't process more than this in a single
// message

#define SERVO_DATA_MAX_LEN 1400
#define MILLI_SECOND_SAMPLE_RATE 200
#define MAX_RESPONSE_SAMPLES 1000/MILLI_SECOND_SAMPLE_RATE*14

namespace gcp {
  namespace antenna {
    namespace control {
      
      
      class ServoComms;
      
      class ServoCommandSa {

      public:
	
	/**
	 * Enumerate supported commands
	 */
	enum Request {

	  INVALID,  // Default which will be used to
	  // indicate that a command has not
	  // been initialized
	  /* These are for the South African dish */
	  SERVO_ENGAGE       ,
	  GET_AZEL           ,
	  GET_PRIOR_LOC      ,
	  SEND_POS           ,
	  SEND_POS_TRIP      ,
	  LOAD_LOOP_PARAMS_A ,
	  LOAD_LOOP_PARAMS_B ,
	  LOAD_LOOP_PARAMS_C ,
	  LOAD_LOOP_PARAMS_D ,
	  QUERY_STATUS       ,
	  AZ_BRAKE_ON        ,
	  AZ_BRAKE_OFF       ,
	  EL_BRAKE_ON        ,
	  EL_BRAKE_OFF       ,
	  CLUTCHES_ON        ,
	  CLUTCHES_OFF       ,
	  AZ_CONTACTORS_ON   ,
	  AZ_CONTACTORS_OFF  ,
	  EL_CONTACTORS_ON   ,
	  EL_CONTACTORS_OFF  ,
	  POSITION_ERRORS


	  // For the South African Dish 
	  // INVALID            = "";
	  // SERVO_ENGAGE       = "SVE";  //  one input, echo output
	  // GET_AZEL           = "GAE";  // no input, two outputs
	  // GET_PRIOR_LOC      = "GIM";  // no input, many outputs
	  // QUERY_STATUS       = "STS";  // no input, one output
	  // SEND_POS           = "AEL";  // two inputs, echo output
	  // LOAD_LOOP_PARAMS_A = "LPRA"; // 6 inputs, echo output
	  // LOAD_LOOP_PARAMS_B = "LPRB"; // 6 inputs, echo output
	  // LOAD_LOOP_PARAMS_C = "LPRC"; // 6 inputs, echo output
	  // LOAD_LOOP_PARAMS_D = "LPRD"; // 6 inputs, echo output
	  // AZ_BRAKE_ON        = "ABN";  // no input, echo output
	  // AZ_BRAKE_OFF       = "ABF";  // no input, echo output
	  // EL_BRAKE_ON        = "EBN";  // no input, echo output
	  // EL_BRAKE_OFF       = "EBF";  // no input, echo output
	  // CLUTCHES_ON        = "CLN";  // no input, echo output
	  // CLUTCHES_OFF       = "CLF";  // no input, echo output
	  // AZ_CONTACTORS_ON   = "AMN";  // no input, echo output
	  // AZ_CONTACTORS_OFF  = "AMF";  // no input, echo output
	  // EL_CONTACTORS_ON   = "EMN";  // no input, echo output
	  // EL_CONTACTORS_OFF  = "EMF";  // no input, echo output
	  // POSITION_ERRORS    = "ERR";  // no input, two outputs
	};

	/**
	 *  Command map
	 */
       	std::map<Request, std::string> commandMap_;

	/**
	 *  Construct the map
	 */
	void constructMap();

	/**
	 * Index pertaining to all request types.
	 */
	unsigned char request_;

	/**
	 * Command to be issued
	 */
	std::string messageToSend_;

	/**
	 * Expected Response
	 */
	std::string expectedResponse_;

	/**
	 * Response Received
	 */
	char responseReceived_[SERVO_DATA_MAX_LEN];

	/**
	 * Constructor.
	 */
	ServoCommandSa();
	
	/**
	 * Destructor.
	 */
	~ServoCommandSa();
	
	/**
	 * General Pack Command
	 */
       	void packCommand(Request req);
       	void packCommand(Request req, std::vector<float>& values);


	/**
	 * Check if box output is valid when one return value.
	 */
       	void checkOneOutput();

	/**
	 * Check if box output is valid when two return values.
	 */
       	void checkTwoOutputs();

	/**
	 * Check if box output is valid when many return values.
	 */
       	void checkManyOutputs();


	/**
	 * Interpret our awful status response
	 */
	void interpretStatusResponse(const char* response);

	/**
	 * Interpret what we get back
	 */
	void interpretResponse();

	/**
	 * Simple check that two strings match.
	 */
	bool simpleValidityCheck();
	
	/**
	 * Return the number of bytes read.
	 */
	size_t responseLength();


	//      private:
	
	friend class ServoComms;

	/**
	 * The size of the command to send.
	 */
	unsigned short cmdSize_; 
	
	/**
	 * True if we are expecting a response to a command
	 */
	bool expectsResponse_;    
	
	/**
	 * If we are expecting a response, how many bytes should be
	 * received?
	 */
	unsigned short responseLength_; 
	
	/**
	 * A buffer into which we can read data returned by the pmac.
	 */
	unsigned char readData_[SERVO_DATA_MAX_LEN];

	/**
	 * A pointer to this buffer may be handed back via calls to
	 * unsigned int* readRegResponse(), above
	 */
	unsigned char tmpBuffer_[SERVO_DATA_MAX_LEN];
	
	/**
	 *  Whether the response is valid 
	 */
	 bool responseValid_;

	/**
	 *  Whether the response value is valid 
	 */
	 bool responseValueValid_;

	/**
	 *  Whether 1 PPS is present
	 */
	 bool ppsPresent_;

	/**
	 *  Value of the response
	 */
	 float responseValue_[MAX_RESPONSE_SAMPLES];

	/**
	 * The size of the command to send.
	 */
	size_t size();
	
	
      }; // End class ServoCommandSa
      
    }; // End namespace control
  }; // End namespace antenna
}; // End namespace gcp

#endif // End #ifndef 
