#ifndef GCP_ANTENNA_CONTROL_ANTENNAROACHMSG_H
#define GCP_ANTENNA_CONTROL_ANTENNAROACHMSG_H

/**
 * @file AntennaRoachMsg.h
 * 
 * Tagged: Mon Nov 23 15:59:36 PST 2009
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/GenericTaskMsg.h"

#include "gcp/control/code/unix/libunix_src/common/netobj.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/specific/Directives.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      /**
       * Class to encapsulate messages send to the Roach task
       */
      class AntennaRoachMsg :
	public gcp::util::GenericTaskMsg {
	
	public:
	
	/**
	 * Enumerate supported AntennaRoach messages
	 */
	enum MsgType {	  
	  INVALID           ,
	  CONNECT           ,
	  DISCONNECT        ,
	  READ_DATA         ,
	  WRITE_DATA        ,
	  ROACH_CMD
	};

	// these are bogus.  we never refer to these.
	enum roachCmdId {
	  ROACH_COMMAND
	};
	
	/**
	 * The type of this message
	 */
	MsgType type;

	/**
	 *  Time of request
	 */
	double currTime_; // in mjd

	/**
	 * Define a Message container
	 */
	union {
	  struct {
	    unsigned cmdId;
	    char* stringCommand;
	    float fltVal;
	    int roachNum;
	  } roach;
	} body;
	
	//------------------------------------------------------------
	// Methods for packing messages
	//------------------------------------------------------------

	//------------------------------------------------------------
	// Pack a command to read data from the receiver
	//------------------------------------------------------------
	inline void packReadDataMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = READ_DATA;
	  // number of roach  = roachNum;
	}

	//------------------------------------------------------------
	// Pack a command to write data from the receiver to disk
	//------------------------------------------------------------
	inline void packWriteDataMsg(gcp::util::TimeVal& currTime)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = WRITE_DATA;

	  currTime_ = currTime.getTimeInSeconds();
	  
	}

	//------------------------------------------------------------
	// Pack a command to connected to receiver backend
	//------------------------------------------------------------
	inline void packConnectMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = CONNECT;
	}

	//------------------------------------------------------------
	// Pack a command to disconnect the receiver
	//------------------------------------------------------------
	inline void packDisconnectMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = DISCONNECT;
	}

	//------------------------------------------------------------
	// Pack a general command
	//------------------------------------------------------------
	inline void packRoachCmdMsg(float fltVal, int roachNumber, char* stringCommand)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = ROACH_CMD;
	  
	  body.roach.fltVal   = fltVal;
	  body.roach.roachNum = roachNumber;
	  body.roach.stringCommand  = stringCommand;
	  // we'll convert the message into what issueCommand wants
	  // in execute command

	};

      }; // End class AntennaRoachMsg

    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
