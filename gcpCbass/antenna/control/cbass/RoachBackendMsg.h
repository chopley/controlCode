#ifndef ROACHBACKENDMSG_H
#define ROACHBACKENDMSG_H

/**
 * @file RoachBackendMsg.h
 * 
 * Tagged: Thu Nov 13 16:53:43 UTC 2003
 * 
 * @author Stephen Muchovej
 */
#include <string>
#include <map>
#include <vector>
#include <netinet/in.h>
#include <errno.h>

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

#include "gcp/util/common/GenericTaskMsg.h"

//#include "gcp/antenna/control/specific/roachserver/structUDPCBASSpkt.h"

#include "gcp/control/code/unix/libunix_src/common/netobj.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/specific/Directives.h"


// The hard limit -- pmac can't process more than this in a single
// message

#define ROACH_DATA_MAX_LEN 40000
#define MILLI_SECOND_SAMPLE_RATE 10
#define MAX_RESPONSE_SAMPLES 1000/MILLI_SECOND_SAMPLE_RATE*6*2*64

// the following should move the specificregs.h eventually
#define NUM_ROACH_INTEGRATION_PER_TRANSFER 10
#define NUM_CHANNELS_PER_BAND 32
#define NUM_PARITY 2

#define kDataperPacket 10
#define vectorLength 32 //size of FFT (32)                                                                                 
#define kMax32bitchans 320 //size of vector to store data (kDataperPacket*vectorLenght

#define CC_TO_SEC 128/250000000                                   


#define DEFAULT_NUMBER_BYTES_PER_TRANSFER 15632
//#define DEFAULT_NUMBER_BYTES_PER_TRANSFER 15472
//#define DEFAULT_NUMBER_BYTES_PER_TRANSFER 15432

namespace gcp {
  namespace antenna {
    namespace control {
      
      
      class RoachBackend;
      
      class RoachBackendMsg :
      public gcp::util::GenericTaskMsg {

      public:

	/**
	 * Constructor.
	 */
	RoachBackendMsg();
	void Assign3DVectorMemory();
	void Assign2DVectorMemory();
	
	/**
	 * Destructor.
	 */
	~RoachBackendMsg();
	
        /**
         *  Time of request 
         */
        double currTime_; // in mjd                                                            
	//	friend class RoachBackend;	
	/**
	 * Enumerate supported commands
	 */
	enum Request {

	  /* need a PACKBLAHBLAH function to deal with each of these. */
	  INVALID,  // Default which will be used to
	  READ_DATA,     // test command
	  WRITE_DATA,
	  CONNECT,
	  DISCONNECT,
	  ROACH_COMMAND
	};

	/**
	 * Index pertaining to all request types.
	 */
	unsigned char request_;

	/**
	 * The size of the command to send.
	 */
	unsigned short cmdSize_; 

	/**
	 * Command to be issued
	 */
	std::string messageToSend_;

	/**
	 * True if we are expecting a response to a command
	 */
	bool expectsResponse_;    

	/**
	 * Expected Response
	 */
	std::string expectedResponse_;
	int numBytesExpected_;

	/**
	 * Response Received
	 */
	char responseReceived_[DEFAULT_NUMBER_BYTES_PER_TRANSFER];
	int numBytesReceived_;

	/**
	 * Return the number of bytes read.
	 */
	size_t responseLength();


	
	/**
	 * General Pack Command
	 */
       	void packCommand(Request req);
       	void packCommand(Request req, std::vector<float>& values);

	/**
	 * Interpret what we get back
	 */
	void interpretResponse();

	/**
	 * Simple check that two strings match.
	 */
	bool simpleValidityCheck();

	/**
	 * A buffer into which we can read data returned by the roach.
	 */
	unsigned char readData_[ROACH_DATA_MAX_LEN];	

	/** 
	 * Actual registers for a given packet.
	 **/
	int version_;   // roach version number
	int packetSize_;// numbers of bytes in the transfer
	int numFrames_; // number of frames being transfer
	int intCount_;  // number of accumulation on fpga
	std::vector<int> bufferBacklog_;    // backlog
	std::vector<int> tstart_;    // start time of integration
	std::vector<int> seconds_;    // roach NTP second
	std::vector<int> useconds_;    // roach NTP usecond
	std::vector<int> switchstatus_;
	int tstop_;     // stop time of integration
	int intLength_; // integration length in clock cycles
	int mode_;      // backend mode (polarization or power);
	int res2_;      // reserved 4 bytes
	
#if(0)
	std::vector<std::vector< std::vector<float> > > LL_;
	std::vector<std::vector<std::vector<float> > > RR_;
	std::vector<std::vector<std::vector<float> > > Q_;
	std::vector<std::vector<std::vector<float> > > U_;
	std::vector<std::vector<std::vector<float> > > TL1_;
	std::vector<std::vector<std::vector<float> > > TL2_;
#endif

	std::vector<std::vector<float> >  LL_;
	std::vector<std::vector<float> >  RR_;
	std::vector<std::vector<float> >  Q_;
	std::vector<std::vector<float> >  U_;
	std::vector<std::vector<float> >  TL1_;
	std::vector<std::vector<float> >  TL2_;


	/**
	 *  Whether the response is valid 
	 */
	 bool responseValid_;
	 
	 /**
	  * Whether the controller said it did it
	  **/
	 bool responseGood_;

	/**
	 *  Whether the response value is valid 
	 */
	 bool responseValueValid_;

	/**
	 *  Value of the response
	 */
	 float responseValue_[MAX_RESPONSE_SAMPLES];

//data structure definition
struct UDPCBASSpkt {
  int version; // 4 byte
  int data_size; // 4 byte
  int dataCount; //4 byte
  int buffBacklog[10]; //40 byte
  int int_count; // 4 byte
  int tstart[10]; // 4 byte
  int tend; // 4 byte
  int int_len; // 4 byte
  int reserved1; // 4 byte
  int reserved2; // 4 byte
  int data_ch0odd[kDataperPacket*vectorLength];
  int data_ch0even[kDataperPacket*vectorLength];
  int data_ch1odd[kDataperPacket*vectorLength];
  int data_ch1even[kDataperPacket*vectorLength];
  int data_ch2odd[kDataperPacket*vectorLength];
  int data_ch2even[kDataperPacket*vectorLength];
  int data_ch3odd[kDataperPacket*vectorLength];
  int data_ch3even[kDataperPacket*vectorLength];
  int data_ch4odd[kDataperPacket*vectorLength];
  int data_ch4even[kDataperPacket*vectorLength];
  int data_ch5odd[kDataperPacket*vectorLength];
  int data_ch5even[kDataperPacket*vectorLength];
  int data_switchstatus[kDataperPacket];
  int secondIntegration[kDataperPacket]; //4*10=40
  int tsecond[10];
  int tusecond[10];
};


	UDPCBASSpkt packet_;

	UDPCBASSpkt* packetPtr_;

	int packetizeNetworkMsg();
	
	void PrintData();

	//------------------------------------------------------------ 
	// Methods for packing messages
        //------------------------------------------------------------ 

        //------------------------------------------------------------  
        // Pack a command to read data from the roach 
        //------------------------------------------------------------ 
        inline void packReadDataMsg()
        {
          genericMsgType_ =
            gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          request_          = READ_DATA;
	  messageToSend_    = "GETDATAXXX";
	  expectsResponse_  = true;
	  numBytesExpected_ = DEFAULT_NUMBER_BYTES_PER_TRANSFER;
	  cmdSize_          = 10;
          // number of rx  = rxNum;
	}


        //------------------------------------------------------------                         
	// Pack a command to write data from the roach to disk
        //------------------------------------------------------------                         
	inline void packWriteDataMsg(gcp::util::TimeVal& currTime)
        {
          genericMsgType_ =
            gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          request_ = WRITE_DATA;

          currTime_ = currTime.getTimeInSeconds();

        }

	//------------------------------------------------------------                         
	// Pack a command to connected to roach
        //------------------------------------------------------------                         
	inline void packConnectMsg()
        {
	    genericMsgType_ =
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;

	    request_ = CONNECT;
        }

        //------------------------------------------------------------                         
        // Pack a command to disconnect the roach
        //------------------------------------------------------------                         
        inline void packDisconnectMsg()
	{
          genericMsgType_ =
            gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          request_ = DISCONNECT;
        }


        //------------------------------------------------------------                         
        // Pack a general command                                                              
        //------------------------------------------------------------                         
        inline void packRoachCmdMsg(std::string stringCommand)
        {
          genericMsgType_ =
            gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          request_ = ROACH_COMMAND;
	  messageToSend_    = stringCommand;
	  expectsResponse_  = true;
	  numBytesExpected_ = 12;
	  cmdSize_          = stringCommand.size();
	  
	};
  



      }; // End class RoachBackendMsg
      
    }; // End namespace control
  }; // End namespace antenna
}; // End namespace gcp

#endif // End #ifndef 
