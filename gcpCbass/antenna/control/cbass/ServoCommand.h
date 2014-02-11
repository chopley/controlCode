#ifndef SERVOCOMMAND_H
#define SERVOCOMMAND_H

/**
 * @file ServoCommand.h
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

#define SERVO_DATA_MAX_LEN 1600
#define MILLI_SECOND_SAMPLE_RATE 200
#define MAX_RESPONSE_SAMPLES 1000/MILLI_SECOND_SAMPLE_RATE*4+2

namespace gcp {
  namespace antenna {
    namespace control {
      
      
      class ServoComms;
      
      class ServoCommand {
      public:
	
	/**
	 * Enumerate supported commands
	 */
	enum Request {

	  /* need a PACKBLAHBLAH function to deal with each of these. */
	  INVALID,  // Default which will be used to
	  // indicate that a command has not
	  // been initialized
	  /* First set is for the Douloi box */
	  GET_AZEL        ,
	  GET_PRIOR_LOC   ,
	  GET_PPS_TICK    ,
	  SEND_POS        ,
	  BEGIN_PPS_LOOP  ,
	  BEGIN_TRACK_LOOP,
	  LOAD_LOOP_PARAMS,
	  STOP_ALL        ,
	  CAL_ENCODERS    ,
	  CLEAR_OFFSETS   ,
	  POSITION_OFFSET ,
	  VELOCITY_OFFSET ,
	  POSITION_ERRORS ,
	  SET_ENCODERS    ,
	  QUERY_STATUS    ,
	  AZ_BRAKE_ON     ,
	  AZ_BRAKE_OFF    ,
	  EL_BRAKE_ON     ,
	  EL_BRAKE_OFF    ,
	  SET_AZ_TORQUE   ,
	  SET_EL_TORQUE   ,
	  QUERY_VERSION   ,
	  SET_AZ_TORQUES  ,
	  READ_AZ_ENCODERS,
	  NUMBER_ERRORS   ,

	  /* Second set is for the electronics card */
	  START_AMP_COMMS  ,
	  ENABLE_AZ_AMP1   ,
	  ENABLE_AZ_AMP2   ,
	  ENABLE_EL_AMP    ,
	  DISABLE_AZ_AMP1  ,
	  DISABLE_AZ_AMP2  ,
	  DISABLE_EL_AMP   ,
	  STATUS_AZ_AMP1    ,
	  STATUS_AZ_AMP2    ,
	  STATUS_EL_AMP    ,
	  ALIVE_AZ_AMP1    ,
	  ALIVE_AZ_AMP2    ,
	  ALIVE_EL_AMP     ,
	  ACTUAL_CURR_AZ1  ,
	  ACTUAL_CURR_AZ2  ,
	  ACTUAL_CURR_EL   ,
	  COMMAND_CURR_AZ1 ,
	  COMMAND_CURR_AZ2 ,
	  COMMAND_CURR_EL  

	  //	  /* First set is for the Douloi box 
	  // INVALID          = '';
	  // GET_AZEL         = 'GAE';
	  // GET_PRIOR_LOC    = 'GIM';
	  // GET_PPS_TICK     = 'GPPS';
	  // SEND_POS         = 'AEL';  // needs further input
	  // BEGIN_PPS_LOOP   = 'TIM';  
	  // BEGIN_TRACK_LOOP = 'TON';
	  // LOAD_LOOP_PARAMS = 'LPR'; //needs further input
	  // STOP_ALL         = 'SPA';
	  // CAL_ENCODERS     = 'CLE';
	  // CLEAR_OFFSETS    = 'CLO';
	  // POSITION_OFFSET  = 'POF'; //needs more input
	  // VELOCITY_OFFSET  = 'VOF'; //needs more input
	  // POSITION_ERRORS  = 'ERR';
	  // SET_ENCODERS     = 'SEN'; //needs more input
	  // QUERY_STATUS     = 'STS';
	  // AZ_BRAKE_ON      = 'ABN';
	  // AZ_BRAKE_OFF     = 'ABF';
	  // EL_BRAKE_ON      = 'EBN';
	  // EL_BRAKE_OFF     = 'EBF';
	  // SET_AZ_TORQUE    = 'AZV'; //needs more input
	  // SET_EL_TORQUE    = 'ELV'; //needs more input
	  // QUERY_VERSION    = 'VER';
	  // SET_AZ_TORQUES   = 'AZS'; //needs more input
	  // READ_AZ_ENCODERS = 'ENC';
	  // REPORT_ERRORS    = 'ERR';
	  // */
	  //
	  // /* Second set is for the electronics card 
	  // START_AMP_COMMS  = '2A01';  // if not completed, no >
	  // ENABLE_AZ_AMP1   = '2A01EN1'; // if not completed, no >
	  // ENABLE_AZ_AMP2   = '2A02EN1'; 
	  // ENABLE_EL_AMP    = '2A03EN1'; 
	  // DISABLE_AZ_AMP1  = '2A01EN0'; // 2A01> if not completed, no >
	  // DISABLE_AZ_AMP2  = '2A02EN0';
	  // DISABLE_EL_AMP   = '2A03EN0';  
	  // QUERY_AZ_AMP1    = '2A01EN';  // 2A01(0-1) for disable/enable
	  // QUERY_AZ_AMP2    = '2A02EN';
	  // STATUS_EL_AMP    = '2A03EN';  
	  // ALIVE_AZ_AMP1    = '2A01RST'; // if 2A01>, the amp is communicating.
	  // ALIVE_AZ_AMP2    = '2A02RST';	 
	  // ALIVE_EL_AMP     = '2A03RST';  
	  // ACTUAL_CURR_AZ1  = '2A01SIA'; // 2A01VAL (in abs term)
	  // ACTUAL_CURR_AZ2  = '2A02SIA';
	  // ACTUAL_CURR_EL   = '2A03SIA';
	  // COMMAND_CURR_AZ1 = '2A01SIC'; // 2A01+-VAL
	  // COMMAND_CURR_AZ2 = '2A02SIC';
	  // COMMAND_CURR_EL  = '2A03SIC';  
	  // */
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
	ServoCommand();
	
	/**
	 * Destructor.
	 */
	~ServoCommand();
	
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
	 * Start Amplifier Communications
	 */
	void packStartAmpComms();

	/**
	 * Enable the El Amplifier
	 */
	void packEnableElAmp();

	/**
	 * Disable the El Amplifier
	 */
	void packDisableElAmp();

	/**
	 * Enable the Az Amplifier 1
	 */
	void packEnableAzAmp1();

	/**
	 * Disable the Az Amplifier 1
	 */
	void packDisableAzAmp1();

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
	
	
      }; // End class ServoCommand
      
    }; // End namespace control
  }; // End namespace antenna
}; // End namespace gcp

#endif // End #ifndef 
