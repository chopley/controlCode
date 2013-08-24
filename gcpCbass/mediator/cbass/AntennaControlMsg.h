#ifndef GCP_ASSEMBLER_ANTENNACONTROLMSG_H
#define GCP_ASSEMBLER_ANTENNACONTROLMSG_H

/**
 * @file AntennaControlMsg.h
 * 
 * Tagged: Thu Nov 13 16:53:57 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/Debug.h"

#include "gcp/util/common/GenericTaskMsg.h"
#include "gcp/antenna/control/specific/AntennaMasterMsg.h"

// Do not move these around.  GCC3.2.2 is easily confused

#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"
#include "gcp/control/code/unix/libunix_src/common/tcpip.h"

namespace gcp {
namespace mediator {
    
    class AntennaControlMsg :
      public gcp::util::GenericTaskMsg {
      
      public:
      
      /**
       * Enumerate supported message types.
       */
      enum MsgType {
	
	// Flag an antenna as reachable/unreachable
	
	FLAG_ANTENNA,
	
	// We are starting/finishing recording initialization sequence

	INIT,

	// The start of an initialization script

	BEGIN_INIT,

	// The end of an initialization script

	END_INIT,

	// A net command to be sent directly to the antenna socket.
	
	NETCMD,
	
	// A message intended for the master task
	
	ANTENNA_MASTER_MSG,

	// Weather data
	
	WEATHER
      };
      
      /**
       * A type for this message
       */
      MsgType type;
      
      /**
       * The contents of the message.
       */
      union {
	
	/**
	 * Flag a set of antennas as (un)reachable
	 */
	struct {
	  unsigned antenna; // The mask of antennas which changed
			    // state
	  bool flag;        // True to flag, false to unflag.
	  
	} flagAntenna;
	
	/**
	 * Tell the antenna control task we are starting/finishing an
	 * initialization sequence
	 */
	struct { 
	  bool start; 
	} init;

	/**
	 * A network command forwarded unmodified from the ACC
	 */
	struct {
	  gcp::control::NetCmdId opcode;
	  gcp::control::RtcNetCmd cmd;
	} netCmd;
	
	/**
	 * A message for the receiver task.
	 */
	gcp::antenna::control::AntennaMasterMsg antennaMasterMsg;
	
      } body;
      
      /**
       * A bit set of antennas to which this message applies.
       */
      gcp::util::AntNum::Id antennas;

      /**
       * True if this is a command generated by an initialization script.
       */
      bool init_;

      //------------------------------------------------------------
      // Methods for returning pointers to messages for threads
      // managed by this task.
      //------------------------------------------------------------
      
      // Don't muck with the value of init_ here -- if it is set, we
      // don't it to be reset just by a call to this method to
      // retrieve it.

      inline gcp::antenna::control::AntennaMasterMsg* getMasterMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;

	  type = ANTENNA_MASTER_MSG;

	  return &body.antennaMasterMsg;
	}
      
      //------------------------------------------------------------
      // Methods for packing messages to the AntennaControl
      // task
      //------------------------------------------------------------
      
      /**
       * A method for packing a message that we are beginning an init
       * script
       */
      inline void packBeginInitMsg() {
	initialize();

	type = BEGIN_INIT;
      }

      /**
       * A method for packing a message that we are ending an init
       * script
       */
      inline void packEndInitMsg() {
	initialize();

	type = END_INIT;
      }

      /**
       * A method for packing an init message
       */
      inline void packInitMsg(bool start) {
	initialize();

	type = INIT;

	body.init.start = start;
      }
      
      /**
       * A method for packing a message to flag an antenna as
       * un/reachable
       */
      inline void packFlagAntennaMsg(unsigned antenna, bool flag) {
	initialize();
	
	DBPRINT(true, gcp::util::Debug::DEBUG6, "");

	type = FLAG_ANTENNA;

	body.flagAntenna.antenna = antenna;
	body.flagAntenna.flag = flag;
      }
      
      /**
       * A method for packing a net command.
       */
      inline void packRtcNetCmdMsg(gcp::control::RtcNetCmd* cmd, 
				   gcp::control::NetCmdId opcode) {
	initialize();

	antennas = (gcp::util::AntNum::Id)cmd->antennas;
	
	type = NETCMD;

	body.netCmd.cmd = *cmd;
	body.netCmd.opcode = opcode;
      }
      
      private:

      // Private initialization method

      inline void initialize() {
	genericMsgType_ = 
	  gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	init_ = false;
      }
	
    }; // End class AntennaControlMsg
    
} // End namespace mediator
} // End namespace gcp

#endif // End #ifndef 