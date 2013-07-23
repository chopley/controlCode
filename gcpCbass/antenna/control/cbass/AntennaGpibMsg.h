// $Id: AntennaGpibMsg.h,v 1.4 2009/08/25 23:40:42 eml Exp $

#ifndef GCP_ANTENNA_CONTROL_ANTENNAGPIBMSG_H
#define GCP_ANTENNA_CONTROL_ANTENNAGPIBMSG_H

/**
 * @file AntennaGpibMsg.h
 * 
 * Tagged: Tue Aug 18 13:11:13 PDT 2009
 * 
 * @version: $Revision: 1.4 $, $Date: 2009/08/25 23:40:42 $
 * 
 * @author tcsh: username: Command not found.
 */
#include "gcp/util/common/GenericTaskMsg.h"

namespace gcp {
  namespace antenna {
    namespace control {

      class AntennaGpibMsg :
	public gcp::util::GenericTaskMsg {

      public:

	enum MsgType {
	  CONNECT,
	  READOUT_THERMO,
	  PROBE_CRYOCON,
	  GPIB_CMD,
	};

	/**
	 * The type of this message
	 */
	MsgType type;

	/**
	 * Define a union of data for all messages handled by the GPIB
	 * task
	 */
	union {

	  struct {
	    unsigned device;
	    unsigned cmdId;
	    int intVals[2];
	    float fltVal;
	  } gpib;

	} body;

	//------------------------------------------------------------
	// Pack a command to connect to the GPIB controller
	//------------------------------------------------------------

	inline void packConnectMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = CONNECT;
	}

	//------------------------------------------------------------
	// Pack a command to readout the thermometry
	//------------------------------------------------------------

	inline void packReadoutMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = READOUT_THERMO;
	}

	//------------------------------------------------------------
	// Pack a GPIB command received from the contrnol system
	//------------------------------------------------------------

	inline void packGpibCmdMsg(unsigned device, unsigned cmdId, int* intVals, float fltVal)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = GPIB_CMD;

	  body.gpib.device     = device;
	  body.gpib.cmdId      = cmdId;
	  body.gpib.intVals[0] = intVals[0];
	  body.gpib.intVals[1] = intVals[1];
	  body.gpib.fltVal     = fltVal;
	}

      }; // End class AntennaGpibMsg

    } // End namespace control
  } // End namespace antenna
} // End namespace gcp



#endif // End #ifndef GCP_ANTENNA_CONTROL_ANTENNAGPIBMSG_H
