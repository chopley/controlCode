#ifndef ANTENNAMASTERMSG_H
#define ANTENNAMASTERMSG_H

/**
 * @file AntennaMasterMsg.h
 * 
 * Tagged: Thu Nov 13 16:53:29 UTC 2003
 * 
 * @author Erik Leitch
 */
#include <string>

#include "gcp/util/common/SignalTask.h"
#include "gcp/util/common/GenericMasterTaskMsg.h"

#include "gcp/antenna/control/specific/AntennaControlMsg.h"
#include "gcp/antenna/control/specific/AntennaDriveMsg.h"
#include "gcp/antenna/control/specific/AntennaMonitorMsg.h"
#include "gcp/antenna/control/specific/AntennaRxMsg.h"
#include "gcp/antenna/control/specific/AntennaRoachMsg.h"
#include "gcp/antenna/control/specific/AntennaTask.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      /**
       * A class to manage messages used to communicate with an
       * AntennaMaster thread.
       */
      class AntennaMasterMsg : 
	public gcp::util::GenericMasterTaskMsg {
	
	public:
	
	/**
	 * Enumerate supported AntennaMaster messages
	 */	
	enum MsgType {
	  CONTROL_MSG,    // A message for the communications task
	  CONTROL_CONNECTED, // We are dis/connected from/to the control host
	  DRIVE_MSG,      // A message for the drive task
	  SCANNER_CONNECTED, // We are dis/connected from/to the archiver
	  MONITOR_MSG,    // A message for the monitor task
	  DRIVE_CONNECTED, // The ACU is dis/connected
	  RX_MSG,         // A message for the rx task
	  RX_CONNECTED,   // A message The rx task is connected/disconnected
	  RX_TIMER,       // A message regarding turnign the RX timers on/off
	  ROACH_MSG,         // A message for the rx task
	  ROACH_CONNECTED,   // A message The rx task is connected/disconnected
	  ROACH_TIMER,       // A message regarding turnign the ROACH timers on/off
	  SEND_HEARTBEAT, // Initiate a heartbeat.
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
	   * A message that the control task is dis/connected from the
	   * control host
	   */
	  struct {
	    bool connected;
	  } controlConnected;


	  /**
	   * A message that the receiver task is dis/connected from the
	   * control host
	   */
	  struct {
	    bool connected;
	  } rxConnected;

	  /**
	   * A message to turn the receiver timers on/off
	   */
	  struct {
	    bool on;
	  } rxTimer;

	  /**
	   * A message that the receiver task is dis/connected from the
	   * control host
	   */
	  struct {
	    bool connected;
	  } roachConnected;

	  /**
	   * A message to turn the receiver timers on/off
	   */
	  struct {
	    bool on;
	  } roachTimer;
	  
	  /**
	   * A message that the scanner is dis/connected from the
	   * archiver host
	   */
	  struct {
	    bool connected;
	  } scannerConnected;
	  
	  /**
	   * A message that the pmac is dis/connected
	   */
	  struct {
	    bool connected;
	  } driveConnected;
	  
	  //------------------------------------------------------------
	  // Message bodies for threads managed by the AntennaMaster
	  //------------------------------------------------------------
	  
	  /**
	   * A message for the AntennaControl task
	   */
	  AntennaControlMsg    controlMsg;   
	  
	  /**
	   * A message for the AntennaDrive task
	   */
	  AntennaDriveMsg    driveMsg;   
	  
	  /**
	   * A message for the AntennaMonitor task
	   */
	  AntennaMonitorMsg  monitorMsg; 
	  
	  /**
	   * A message for the AntennaRx task
	   */
	  AntennaRxMsg       rxMsg;        

	  /**
	   * A message for the AntennaRoach task
	   */
	  AntennaRoachMsg       roachMsg;        

	} body;
	
	//------------------------------------------------------------
	// Methods for accessing task messages managed by this class.
	//------------------------------------------------------------
	
	inline AntennaControlMsg* getControlMsg()
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = CONTROL_MSG;
	    return &body.controlMsg;
	  }
	
	inline AntennaDriveMsg* getDriveMsg()
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = DRIVE_MSG;
	    return &body.driveMsg;
	  }
	
	inline AntennaMonitorMsg* getMonitorMsg()
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = MONITOR_MSG;
	    return &body.monitorMsg;
	  }
	
	inline AntennaRxMsg* getRxMsg()
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = RX_MSG;
	    return &body.rxMsg;
	  }

	inline AntennaRoachMsg* getRoachMsg()
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = ROACH_MSG;
	    return &body.roachMsg;
	  }

	//------------------------------------------------------------
	// Methods for packaging messages intended for this task.
	//------------------------------------------------------------
	
	/**............................................................
	 * Pack a message to send a heartbeat.
	 */
	inline void packSendHeartBeatMsg()
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = SEND_HEARTBEAT;
	  }
	
	inline void packControlConnectedMsg(bool connected)
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = CONTROL_CONNECTED;
	    
	    body.controlConnected.connected = connected;
	  }

	inline void packRxConnectedMsg(bool connected)
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = RX_CONNECTED;
	    
	    body.rxConnected.connected = connected;
	  }

	inline void packRxTimerMsg(bool timeOn)
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = RX_TIMER;
	    
	    body.rxTimer.on = timeOn;
	  }

	inline void packRoachConnectedMsg(bool connected)
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = ROACH_CONNECTED;
	    
	    body.roachConnected.connected = connected;
	  }

	inline void packRoachTimerMsg(bool timeOn)
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = ROACH_TIMER;
	    
	    body.roachTimer.on = timeOn;
	  }
	
	inline void packScannerConnectedMsg(bool connected)
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = SCANNER_CONNECTED;
	    
	    body.scannerConnected.connected = connected;
	  }
	
	inline void packDriveConnectedMsg(bool connected)
	  {
	    genericMsgType_ = 
	      gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	    type = DRIVE_CONNECTED;
	    
	    body.driveConnected.connected = connected;
	  }
	  
      }; // End class AntennaMasterMsg
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
