#ifndef GCP_ANTENNA_CONTROL_ANTENNARXMSG_H
#define GCP_ANTENNA_CONTROL_ANTENNARXMSG_H

/**
 * @file AntennaRxMsg.h
 * 
 * Tagged: Mon Nov 23 15:59:36 PST 2009
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/GenericTaskMsg.h"

#include "gcp/control/code/unix/libunix_src/common/netobj.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/specific/Directives.h"

//#include "gcp/util/specific/Rx.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      /**
       * Class to encapsulate messages send to the Rx task
       */
      class AntennaRxMsg :
	public gcp::util::GenericTaskMsg {
	
	public:
	
	/**
	 * Enumerate supported AntennaRx messages
	 */
	enum MsgType {	  
	  INVALID           ,
	  CONNECT           ,
	  DISCONNECT        ,
	  READ_DATA         ,
	  WRITE_DATA        ,
	  RX_CMD
	};

	// these are bogus.  we never refer to these.
	enum rxCmdId {
	  RX_SETUP_ADC,
	  RX_RESET_FPGA,
	  RX_RESET_FIFO,
	  RX_SET_SWITCH_PERIOD,
	  RX_SET_BURST_LENGTH,
	  RX_SET_INTEGRATION_PERIOD,
	  RX_SET_TRIM_LENGTH,
	  RX_ENABLE_SIMULATOR,
	  RX_ENABLE_NOISE,
	  RX_ENABLE_WALSHING,
	  RX_ENABLE_ALT_WALSHING,
	  RX_ENABLE_FULL_WALSHING,
	  RX_ENABLE_NONLINEARITY,
	  RX_GET_BURST_DATA,
	  RX_ENABLE_ALPHA,
	  RX_SET_ALPHA,
	  RX_SET_NONLIN
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
	    //	    unsigned char address;
	    //    unsigned char* period;
	    float fltVal;
	    int chanVal;
	    int stageVal;
	  } rx;
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
	inline void packRxCmdMsg(unsigned cmdId, float fltVal, int stageVal, int channelVal)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  COUT("got to pack rx md msg");
	  type = RX_CMD;
	  
	  body.rx.cmdId    = cmdId;
	  body.rx.fltVal   = fltVal;
	  body.rx.stageVal = stageVal;
	  body.rx.chanVal  = channelVal;
	  // we'll convert the message into what issueCommand wants
	  // in execute command

	};

      }; // End class AntennaRxMsg

    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
