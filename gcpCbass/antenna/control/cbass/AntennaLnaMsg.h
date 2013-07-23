#ifndef GCP_ANTENNA_CONTROL_ANTENNALNAMSG_H
#define GCP_ANTENNA_CONTROL_ANTENNALNAMSG_H

/**
 * @file AntennaLnaMsg.h
 * 
 * Tagged: Mon Nov 23 15:59:36 PST 2009
 * 
 * @author Stephen Muchovej
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
      class AntennaLnaMsg :
	public gcp::util::GenericTaskMsg {
	
	public:
	
	/**
	 * Enumerate supported AntennaRx messages
	 */
	enum MsgType {	  
	  READ_DATA,
	  CONNECT,
	  DISCONNECT,
	  LNA_CMD
	};

	enum lnaCmdId {
	  LNA_SET_DRAIN_VOLTAGE,
	  LNA_SET_DRAIN_CURRENT,
	  LNA_SET_GATE_VOLTAGE,
	  LNA_SET_BIAS,
	  LNA_SET_MODULE,
	  LNA_CHANGE_VOLTAGE,
	  LNA_GET_VOLTAGE,
	  LNA_ENABLE_BIAS_QUERY
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
	    unsigned lnaCmdId;
	    float drainCurrent;
	    float drainVoltage;
	    int lnaNumber;
	    int stageNumber;
	  } lna;
	} body;
	
	//------------------------------------------------------------
	// Methods for packing messages
	//------------------------------------------------------------

	//------------------------------------------------------------
	// Pack a command to read data from the lna power supply
	//------------------------------------------------------------
	inline void packReadDataMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = READ_DATA;
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
	inline void packLnaCmdMsg(unsigned cmdId, float drainVoltage, float drainCurrent, int lnaNumber, int stageNumber)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  COUT("got to pack lna cmd msg");
	  type = LNA_CMD;
	  
	  body.lna.lnaCmdId       = cmdId;
	  body.lna.drainVoltage   = drainVoltage;
	  body.lna.drainCurrent   = drainCurrent;
	  body.lna.lnaNumber      = lnaNumber;
	  body.lna.stageNumber    = stageNumber;
	  // we'll convert the message into what issueCommand wants
	  // in execute command

	};

      }; // End class AntennaRxMsg

    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
