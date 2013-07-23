#ifndef GCP_UTIL_NETMSG_H
#define GCP_UTIL_NETMSG_H

/**
 * @file NetMsg.h
 * 
 * Tagged: Mon Mar 15 15:29:07 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/NetSendStr.h"
#include "gcp/util/common/Mutex.h"

#include <string.h>

#include "gcp/control/code/unix/libunix_src/common/netobj.h"
#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"

namespace gcp {
  namespace util {
    
    class NetMsg {
    public:
      
      /**
       * Enumerate supported message types.
       */
      enum MsgType {
	
	// A message to be logged
	
	GREETING = gcp::control::NET_GREETING_MSG, 
	
	// A message to be logged
	
	LOG = gcp::control::NET_LOG_MSG, 
	
	// An antenna ID
	
	ID  = gcp::control::NET_ID_MSG,  
	
	// A request for updates of phemeris positions from the
	// navigator thread
	
	NAV_UPDATE = gcp::control::NET_NAV_UPDATE_MSG,  
	
	// Report the completion of a pmac transaction
	
	DRIVE_DONE =  gcp::control::NET_DRIVE_DONE_MSG,   
	
	// Report the completion of a bench transaction
	
	BENCH_DONE =  gcp::control::NET_BENCH_DONE_MSG,   
	
	// Report the completion of a scan transaction
	
	SCAN_DONE =  gcp::control::NET_SCAN_DONE_MSG,   

	// Report that a source has set
	
	SOURCE_SET =  gcp::control::NET_SOURCE_SET_MSG,   
	
	// Report the completion of a caltert transaction
	
	CALTERT_DONE =  gcp::control::NET_CALTERT_DONE_MSG,   
	
	// Report the completion of a caltert transaction
	
	IFMOD_DONE =  gcp::control::NET_IFMOD_DONE_MSG,   
	
	// Report the completion of a CAN transaction
	
	CAN_DONE =  gcp::control::NET_CAN_DONE_MSG,   

	// Report the completion of a SCRIPT transaction
	
	SCRIPT_DONE =  gcp::control::NET_SCRIPT_DONE_MSG,   
      };
      
      /**
       * A type for this message
       */
      MsgType type;
      
      /**
       * The contents of the message.
       */
      gcp::control::RtcNetMsg body;
      
      //------------------------------------------------------------
      // Set the antenna id.
      //------------------------------------------------------------
      
      inline void setAntId(AntNum::Id antId) {
	body.antenna = antId;
      }
      
      //------------------------------------------------------------
      // Methods to pack Network messages
      //------------------------------------------------------------
      
      //------------------------------------------------------------
      // Method to pack a greeting to an antenna
      
      inline void packGreetingMsg(unsigned int id, 
				  unsigned revision,
				  unsigned nReg,
				  unsigned nByte) {
	
	type = GREETING;
	
	body.antenna = id;
	body.msg.greeting.revision = revision;
	body.msg.greeting.nReg     = nReg;
	body.msg.greeting.nByte    = nByte;
      }
      
      
      //------------------------------------------------------------
      // Method to pack a log message
      
      inline unsigned maxMsgLen() {
	return  gcp::control::NET_LOG_MAX;
      }
	
      //------------------------------------------------------------
      // Method to pack a log message
      
      inline void packLogMsg(std::string message, bool isError, 
			     unsigned seq=0, bool end=0) {
	
	int length = message.length();
	
	length = (length > gcp::control::NET_LOG_MAX) ? 
	  gcp::control::NET_LOG_MAX : length;
	
	type = LOG;
	
	strncpy(body.msg.log.text, message.c_str(), length);
	
	// Make sure the string is properly terminated
	
	body.msg.log.text[length] = '\0';
	body.msg.log.bad   = isError;
	body.msg.log.seq   = seq;
	body.msg.log.end   = end;
      }
      
      //------------------------------------------------------------
      // Method to pack an antenna id
      
      inline void packAntennaIdMsg(unsigned int id) {
	
	type = ID;
	body.antenna = id;
      }
      
      //------------------------------------------------------------
      // Method to pack a request for ephemeris updates from the
      // navigator thread
      
      inline void packNavUpdateMsg() {
	type = NAV_UPDATE;
      }
      
      //------------------------------------------------------------
      // Method to pack a pmacxo transaction completion message
      
      inline void packDriveDoneMsg(unsigned seq) {
	type = DRIVE_DONE;
	body.msg.drive_done.seq = seq;
      }
      
      //------------------------------------------------------------
      // Method to pack a bench transaction completion message
      
      inline void packBenchDoneMsg(unsigned seq) {
	type = BENCH_DONE;
	body.msg.bench_done.seq = seq;
      }
      
      //------------------------------------------------------------
      // Method to pack a scan completion message
      
      inline void packScanDoneMsg(unsigned seq) {
	type = SCAN_DONE;
	body.msg.scan_done.seq = seq;
      }

      //------------------------------------------------------------
      // Method to pack a script completion message
      
      inline void packScriptDoneMsg(unsigned seq) {
	type = SCRIPT_DONE;
	body.msg.scriptDone.seq = seq;
      }

      //------------------------------------------------------------
      // Method to report that a source has set
      
      inline void packSourceSetMsg(unsigned seq) {
	type = SOURCE_SET;
	body.msg.source_set.seq = seq;
      }
      
      //------------------------------------------------------------
      // Method to pack a CalTert transaction completion message
      
      inline void packCalTertDoneMsg(unsigned seq) {
	type = CALTERT_DONE;
	body.msg.calTertDone.seq = seq;
      }
      
      //------------------------------------------------------------
      // Method to pack a IFMod transaction completion message
      
      inline void packIFModDoneMsg(unsigned seq) {
	type = IFMOD_DONE;
	body.msg.IFModDone.seq = seq;
      }
      
      //------------------------------------------------------------
      // Method to pack a CAN transaction completion message
      
      inline void packCanCommandDoneMsg(unsigned seq) {
	type = CAN_DONE;
	body.msg.canDone.seq = seq;
      }
      
      inline friend std::ostream& operator<<(std::ostream& os, const NetMsg& rhs) {
        switch (rhs.type) {
	case GREETING:
	  os << std::endl <<
	        "GreetingMsg" << std::endl <<
	        "antenna " << rhs.body.antenna << std::endl <<
	        "revision " << rhs.body.msg.greeting.revision << std::endl <<
	        "nReg " << rhs.body.msg.greeting.nReg << std::endl <<
	        "nByte " << rhs.body.msg.greeting.nByte << std::endl;
          break;
	default:
	  os << "Other NetMsg" << std::endl;
	  break;
	}
      } 
    }; // End class NetMsg
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_NETMSG_H
