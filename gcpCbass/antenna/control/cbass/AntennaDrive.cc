#define __FILEPATH__ "antenna/control/specific/AntennaDrive.cc"

// C++ includes

#include <iostream>
#include <list>

// C includes

#include <ctime>
#include <sys/time.h>
#include <csignal>
#include <csetjmp>
#include <cerrno>
#include <sys/types.h>
#include <unistd.h>

// SZA includes

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/Debug.h"

#include "gcp/antenna/control/specific/AntennaDrive.h"
#include "gcp/antenna/control/specific/AntennaMaster.h"
#include "gcp/antenna/control/specific/EmlDebug.h"

#include "gcp/antenna/control/specific/Tracker.h" 

using namespace std;
using namespace gcp::antenna::control;
using namespace gcp::util;

//-----------------------------------------------------------------------
// Constructor/Destructors

/**.......................................................................
 * Create a AntennaDrive class in namespace carma
 */
AntennaDrive::AntennaDrive(AntennaMaster* parent) : 
  SpecificTask(), gcp::util::GenericTask<AntennaDriveMsg>::GenericTask()
{
  // Initialize member pointers to NULL

  parent_       = 0;
  trackertask_  = 0;

  // Sanity check arguments

  if(parent == 0)
    throw Error("AntennaDrive::AntennaDrive: Received NULL parent argument\n");

  // Keep a pointer to the AntennaMaster object for use in sending
  // messages to the AntennaMaster message queue

  parent_  = parent;

  simPmac_ = parent->simPmac();

  // Keep a pointer to the shared resource object of the parent task.

  share_ = parent_->getShare();

  if(share_ == 0)
    throw Error("AntennaDrive::AntennaDrive: "
		"share argument is NULL.\n");

  // Initialize the array of threads managed by this task.  We don't
  // start these until all of our resources have been allocated.
  
  if(parent->usePrio()) {
    const int TRACKER_PRIORITY = 99;
    threads_.push_back(new Thread(&startTracker, &cleanTracker, 
				  &pingTracker,  "Tracker", 0, 0, 
				  TRACKER_PRIORITY));
  } else {
    threads_.push_back(new Thread(&startTracker, &cleanTracker, 
				  &pingTracker,  "Tracker", 0, 0));
  }

  // Start up threads managed by this task.

  COUT("Starting drive threads");
  startThreads(this);
  COUT("Starting drive threads: done");
};

/**.......................................................................
 * Destructor
 */
AntennaDrive::~AntennaDrive() {};

//-----------------------------------------------------------------------
// Communication methods

/**.......................................................................
 * Process a message received on our message queue
 */
void AntennaDrive::processMsg(AntennaDriveMsg* msg)
{
  switch (msg->type) {
  case AntennaDriveMsg::TRACKER_MSG:
    forwardTrackerMsg(msg);
    break;
  
  default:
    ThrowError("Unrecognized message type: " << msg->type);
    break;
  }; 
}
        
/**.......................................................................
 * Tracker thread startup function
 */
THREAD_START(AntennaDrive::startTracker)
{
  bool waserr=false;
  AntennaDrive* drive = (AntennaDrive*) arg;
  Thread* thread = 0;

  try {
    // Get the Thread object which will manage this thread.
    
    thread = drive->getThread("Tracker");
    
    // Instantiate the subsystem object
    
    drive->trackertask_ = new gcp::antenna::control::Tracker(drive);
    
    // Set our internal thread pointer pointing to the Tracker thread
    
    drive->trackertask_->thread_ = thread;
    
    // Let other threads know we are ready
    
    thread->broadcastReady();
    
    // Finally, block, running our message service:
    
    drive->trackertask_->run();
  } catch(Exception& err) {
    DBPRINT(true, Debug::DEBUGANY, "====== Exception: ====== " << err.what());
    throw err;
  }

  return 0;
}

/**.......................................................................
 * A cleanup handler for the Tracker thread.
 */
THREAD_CLEAN(AntennaDrive::cleanTracker)
{
  AntennaDrive* drive = (AntennaDrive*) arg;

  // Call the destructor function explicitly for rxtask
    
  if(drive->trackertask_ != 0) {
    delete drive->trackertask_;
    drive->trackertask_ = 0;
  }
}

/**.......................................................................
 * A function by which we will ping the Tracker thread
 */
THREAD_PING(AntennaDrive::pingTracker)
{
  bool waserr=0;
  AntennaDrive* drive = (AntennaDrive*) arg;

  if(drive == 0)
    throw Error("AntennaDrive::pingTracker: NULL argument.\n");

  drive->trackertask_->sendHeartBeatMsg();
}

/**.......................................................................
 * We will piggyback on the heartbeat signal received from the parent
 * to send a heartbeat to our own tasks.
 */
void AntennaDrive::respondToHeartBeat()
{
  // Respond to the parent heartbeat request

  if(thread_ != 0)
    thread_->setRunState(true);

  // And ping any threads we are running.

  if(threadsAreRunning()) {
    pingThreads(this);
  } else {
    sendRestartMsg();
  } 
}

/**.......................................................................
 * Forward a message to the pmac task
 */
ANTENNADRIVE_TASK_FWD_FN(AntennaDrive::forwardTrackerMsg)
{
  DBPRINT(true, Debug::DEBUG29, "");
  if(trackertask_ == 0)
    throw Error("AntennaDrive::forwardTrackerMsg: "
		"Tracker task pointer is NULL.\n");

  trackertask_->sendTaskMsg(msg->getTrackerMsg());
}

/**.......................................................................
 * Send a message to the parent that the drive is connected
 */
void AntennaDrive::sendDriveConnectedMsg(bool connected)
{
  AntennaMasterMsg msg;

  msg.packDriveConnectedMsg(connected);

  parent_->forwardMasterMsg(&msg);
}

/**.......................................................................
 * Send a message to the parent that the pmac is disconnected
 */
void AntennaDrive::sendDriveDoneMsg(unsigned int seq)
{
  AntennaMasterMsg msg;
  NetMsg* netMsg = msg.getControlMsg()->getNetMsg();

  netMsg->setAntId(parent_->getAnt()->getId());
  netMsg->packDriveDoneMsg(seq);

  parent_->forwardMasterMsg(&msg);
}

/**.......................................................................
 * Send a message to the parent that the pmac is disconnected
 */
void AntennaDrive::sendBenchDoneMsg(unsigned int seq)
{
  AntennaMasterMsg msg;
  NetMsg* netMsg = msg.getControlMsg()->getNetMsg();

  netMsg->setAntId(parent_->getAnt()->getId());
  netMsg->packBenchDoneMsg(seq);

  parent_->forwardMasterMsg(&msg);
}

/**.......................................................................
 * Send a message to the parent that the scan has completed
 */
void AntennaDrive::sendScanDoneMsg(unsigned int seq)
{
  AntennaMasterMsg msg;
  NetMsg* netMsg = msg.getControlMsg()->getNetMsg();

  netMsg->setAntId(parent_->getAnt()->getId());
  netMsg->packScanDoneMsg(seq);

  parent_->forwardMasterMsg(&msg);
}

/**.......................................................................
 * Send a message to the parent that the source has set.
 */
void AntennaDrive::sendSourceSetMsg(unsigned int seq)
{
  AntennaMasterMsg msg;
  NetMsg* netMsg = msg.getControlMsg()->getNetMsg();

  netMsg->setAntId(parent_->getAnt()->getId());
  netMsg->packSourceSetMsg(seq);

  parent_->forwardMasterMsg(&msg);
}

/**.......................................................................
 * Send a message to the parent that the source has set.
 */
void AntennaDrive::sendPollGpsStatusMsg()
{
  AntennaDriveMsg driveMsg;
  driveMsg.packPollGpsStatusMsg();
  sendTaskMsg(&driveMsg);
}

bool AntennaDrive::simPmac()
{
  return simPmac_;
}
