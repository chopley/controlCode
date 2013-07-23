#ifndef ANTENNADRIVEMSG_H
#define ANTENNADRIVEMSG_H

/**
 * @file AntennaDriveMsg.h
 * 
 * Tagged: Thu Nov 13 16:53:28 UTC 2003
 * 
 * @author Erik Leitch
 */
#include <string>

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/GenericTaskMsg.h"

#include "gcp/antenna/control/specific/TrackerMsg.h"

#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"

namespace gcp {
  namespace antenna {
    namespace control {      
      
      /**
       * A container for messages sent to the Drive Task.
       */
      class AntennaDriveMsg :
	public gcp::util::GenericTaskMsg {
	
	public:
	
	/**
	 * Enumerate supported messages for this task
	 */
	enum MsgType {
	  FLAG_BOARD,        // A message to flag a board.
	  DRIVE_CONNECTED,    // A message from the Tracker task that
	  // the drive is dis/connected.
	  POLL_GPS_STATUS,
	  TRACKER_MSG // A message for the tracker thread.
	};
	
	/**
	 * The type of this message
	 */
	MsgType type;
	
	/**
	 * A union of supported messages
	 */
	union {
	  
	  /**
	   * Flag a board.
	   */
	  struct {
	    unsigned short board; // The register map index of the
	    // board to un/flag
	    bool flag;            // True to flag, false to unflag
	  } flagBoard;
	  
	  /**
	   * A message that the pmac is dis/connected
	   */
	  struct {
	    bool connected;
	  } driveConnected;
	  
	  /**
	   * A message for the Tracker thread.
	   */
	  TrackerMsg trackerMsg;
	  
	} body;
	
	//------------------------------------------------------------
	// Methods for accessing messages for tasks managed by this
	// class.
	//------------------------------------------------------------
	
	inline TrackerMsg* getTrackerMsg()
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = TRACKER_MSG;
	    return &body.trackerMsg;
	  }
	  
	inline AntennaDriveMsg* packShutdownDriveMsg()
	  {
	    TrackerMsg* trackerMsg = getTrackerMsg();
	    
	    trackerMsg->packShutdownDriveMsg();
	  }
	  
	inline AntennaDriveMsg* packPollGpsStatusMsg()
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = POLL_GPS_STATUS;
	  }	
	//------------------------------------------------------------
	// Methods for packing messsages to the drive task.
	//------------------------------------------------------------
	
	inline void packFlagBoardMsg(unsigned short board, bool flag) {
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = FLAG_BOARD;
	  body.flagBoard.board = board;
	  body.flagBoard.flag = flag;
	}
	
	inline void packDriveConnectedMsg(bool connected)
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = DRIVE_CONNECTED;
	    
	    body.driveConnected.connected = connected;
	  }
	
      }; // End AntennaDriveMsg class
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
