#ifndef DRIVETASK_H
#define DRIVETASK_H

/**
 * @file AntennaDrive.h
 * 
 * Tagged: Thu Nov 13 16:53:27 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/SignalTask.h" // SIGNALTASK_HANDLER_FN

#include "gcp/antenna/control/specific/AntennaDriveMsg.h"
#include "gcp/antenna/control/specific/SpecificShare.h"
#include "gcp/antenna/control/specific/SpecificTask.h"
#include "gcp/antenna/control/specific/Tracker.h"

#define ANTENNADRIVE_TASK_FWD_FN(fn) void (fn)(AntennaDriveMsg* msg)

// Create an AntennaDrive class in namespace carma

namespace gcp {
  namespace antenna {
    namespace control {
      
      /**
       * Incomplete type specification for AntennaMaster lets us declare
       * it as a friend below without defining it
       */
      class AntennaMaster;
      
      /**
       * The AntennaDrive collects together the functionality of the base
       * class Drive and its descendants, namely PointingModel.
       * Remotely, there will be a single CORBA DO for an Antenna
       * object, from which antenna->Drive()->Pointingmodel() and its
       * methods can be accessed directly.  However, we want these
       * methods to be routed through the AntennaDrive message queue, and
       * so the antenna->Drive() portion of the DO is managed by the
       * AntennaDrive class.
       */
      class AntennaDrive : 
	public SpecificTask,
	public gcp::util::GenericTask<AntennaDriveMsg> {
	
	public:
	
	/**
	 * Send a message that the pmac is dis/connected
	 */
	void  sendDriveConnectedMsg(bool connected);
	
	/**
	 * Send a message to the parent that the pmac is disconnected
	 */
	void sendDriveDoneMsg(unsigned int seq);
	
	/**
	 * Send a message to the parent that the bench is on target
	 */
	void sendBenchDoneMsg(unsigned int seq);
	
	/**
	 * Send a message to the parent that the scan has finished
	 */
	void sendScanDoneMsg(unsigned int seq);

	/**
	 * Send a message to the parent that the source has set.
	 */
	void sendSourceSetMsg(unsigned int seq);
	
	void sendPollGpsStatusMsg();
	
	bool simPmac();

	private:
	
	bool simPmac_;

	/**
	 * We declare AntennaMaster a friend because its
	 * startAntennaDrive() method will call serviceMsgQ().
	 */
	friend class AntennaMaster;
	
	/**
	 * Pointer to the parent task resources
	 */
	AntennaMaster* parent_;
	
	/**
	 * Private constructor prevents instantiation by anyone but
	 * AntennaMaster.
	 *
	 * @throws Exception
	 */
	AntennaDrive(AntennaMaster* parent);
	
	/**
	 * Destructor.
	 *
	 * @throws Exception
	 */
	~AntennaDrive();
	
	/**
	 * Process a message received on the AntennaDrive message queue
	 *
	 * @throws Exception
	 */
	void processMsg(AntennaDriveMsg* taskMsg);
	
	/**
	 * Tell the Antenne Task we want to take responsibility for
	 * flagging/unflagging operations for our own boards.
	 *
	 * @throws Exception
	 */
	void adoptBoards();
	
	//------------------------------------------------------------
	// Thread management functions.
	//------------------------------------------------------------
	
	/**
	 * Startup routine for the pmac task thread
	 */
	static THREAD_START(startTracker);
	
	//------------------------------------------------------------
	// Thread cleanup handlers methods
	
	/**
	 * Cleanup routine for the pmac task thread
	 */
	static THREAD_CLEAN(cleanTracker);
	
	//------------------------------------------------------------
	// Ping routines for the (pingable) subsystem threads
	
	/**
	 * Ping routine for the pmac task thread
	 */
	static THREAD_PING(pingTracker);
	
	//------------------------------------------------------------
	// Subsystem resources.
	//
	// Pointers to the subsystem resources.  These pointers are
	// initialized by the subsystem threads on startup
	
	/**
	 * Pointer to the pmac system resources
	 */
	Tracker* trackertask_;
	
	/**
	 * Method for forwarding a message to the Tracker task.
	 */
	ANTENNADRIVE_TASK_FWD_FN(forwardTrackerMsg);
	
	/**
	 * Override GenericTask::respondToHeartBeat()
	 */
	void respondToHeartBeat();
	
      }; // End class AntennaDrive
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif


