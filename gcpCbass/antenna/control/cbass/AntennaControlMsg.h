#ifndef SZA_ANTENNA_ANTENNACONTROLMSG_H
#define SZA_ANTENNA_ANTENNACONTROLMSG_H

/**
 * @file AntennaControlMsg.h
 * 
 * Started: Thu Feb 26 14:19:05 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/GenericTaskMsg.h"
#include "gcp/util/common/NetMsg.h"

#include "gcp/antenna/control/specific/AntennaGpibMsg.h"
#include "gcp/antenna/control/specific/AntennaLnaMsg.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      class AntennaControlMsg : 
	public gcp::util::GenericTaskMsg {
	
	public:
	
	/**
	 * Enumerate supported message types.
	 */
	enum MsgType {
	  CONNECT,   // Attempt to connect to the control host.
	  NETMSG,    // A network message intended for the controller.
	  GPIB_MSG,  // A message intended for the gpib network
	  DLP_TEMPS, // A message intended for the temperature controller
	  LNA_MSG,   // A message to the LNA power supply
	  ADC_VOLTS,   // A message to the Labjack ADC
	};
	
	/**
	 * A type for this message
	 */
	MsgType type;
	
	union {
	  gcp::util::NetMsg networkMsg;
	  gcp::antenna::control::AntennaGpibMsg gpibMsg;
	  gcp::antenna::control::AntennaLnaMsg lnaMsg;
	} body;

	/**
	 * Method to get a pointer to the NetMsg
	 */
	inline gcp::util::NetMsg* getNetMsg()
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = NETMSG;
	    return &body.networkMsg;
	  }

	//------------------------------------------------------------
	// Methods for accessing task messages managed by this class.
	//------------------------------------------------------------
	
	inline AntennaGpibMsg* getGpibMsg()
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = GPIB_MSG;
	    return &body.gpibMsg;
	  }

	inline AntennaLnaMsg* getLnaMsg()
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = LNA_MSG;
	    return &body.lnaMsg;
	  }

	//------------------------------------------------------------
	// Methods for packing message to the communications task.
	//
	// We explicitly initialize genericMsgType_ in each method,
	// since we cannot do this in a constructor, since objects
	// with explicit constructors apparently can't be union
	// members.
	//------------------------------------------------------------
	
	/**
	 * Pack a message to connect to the pmac.
	 */
	inline void packConnectMsg() {
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = CONNECT;
	}

	inline void packDlpMsg() {
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = DLP_TEMPS;
	}

	inline void packAdcMsg() {
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = ADC_VOLTS;
	}
	
	private:
      }; // End class AntennaControlMsg
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 


