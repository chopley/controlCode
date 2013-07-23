#ifndef SZA_TRANSLTOR_MASTER_H
#define SZA_TRANSLTOR_MASTER_H

/**
 * @file Master.h
 * 
 * Tagged: Thu Nov 13 16:53:58 UTC 2003
 * 
 * @author Erik Leitch
 */
#include <vector>

#include "gcp/mediator/specific/MasterMsg.h"

#include "gcp/util/specific/Directives.h"

#include "gcp/util/common/ArrayRegMapDataFrameManager.h"
#include "gcp/util/common/GenericMasterTask.h"
#include "gcp/util/common/SignalTask.h"

#define HEARTBEAT_SIGNAL       SIGRTMIN
#define DATAFRAME_SIGNAL       SIGRTMIN+1
#define CONNECT_SIGNAL         SIGRTMIN+2

#define HEARTBEAT_SEC   60
#define DATAFRAME_NSEC  1000000000 // Every second
#define CONNECT_SEC     2
#define WX_FRAME_COUNT  60  // Read the weather station every 30 seconds
			    // (60 half-second frames)

#define MASTER_TASK_FWD_FN(fn) void (fn)(MasterMsg* msg)

namespace gcp {
namespace mediator {
    
    /**
     * Incomplete declarations for classes needed by .
     */
    class Control;
    class Scanner;
    
    /**............................................................
     * A class to encapsulate translation between the old-style DASI
     * array control code and the new SZA antenna code.  DASI code
     * uses proprietary socket communications & network message
     * containers to relay command messages and data between the ACC
     * and the AC, while SZA code uses distributed CORBA objects and
     * event channels.
     *
     * Instantiating a  object starts up five threads: the
     * main one, and four spawned threads, represented by the
     * following objects:
     *
     *   1) Control: which listens to the control port
     *   for network commands, and translates them into
     *   commands distributed to various tasks
     *
     *   2) Scanner: which receives monitor events from the antenna
     *   task, translates them into data frames, and packages them for
     *   transmission to the control archiver port.
     *
     *   3) Signal: which handles all signals for this task.
     */
    class Master : 
      public gcp::util::GenericMasterTask<MasterMsg> { 
      
      public:
      
      //============================================================
      // Master public members & methods
      //============================================================
      
      /**
       * Constructor.
       *
       * @throws Exception
       */
      Master(std::string ctlHost,
	     std::string wxHost,
	     bool sim,
	     bool simRx);

      /**
       * Destructor.
       */
      ~Master();
      
      //------------------------------------------------------------
      // All task messages will be sent to the Master via
      // the following message forwarding function.  On receipt of a
      // message, the master will determine which task is the
      // intended recipient, and forward it appropriately via the
      // private task forwarding methods below.
      //
      // The idea is that when a task is spawned by
      // Master, it is guaranteed that the
      // Master msgQ already exists, while there is no
      // guarantee that the msgQs of the other tasks have yet been
      // created. 
      //
      // By forwarding all messages through the master, we are
      // therefore guaranteed that a task will never attempt to
      // access the message queue or methods of another task before
      // the resources of that task have been allocated.  We are
      // also guaranteed that a message will never be lost because
      // all messages will be queued until the master runs its
      // serviceMsgQ() method, which happens only after all spawned
      // tasks are running theirs.
      
      /**
       * Method by which tasks can forward messages to us.  
       */
      MASTER_TASK_FWD_FN(forwardMasterMsg);
      
      /**
       * Methods by which tasks can query std::strings pertinent to
       * the connection.
       */
      std::string ctlHost() {
	return ctlHost_;
      }

      std::string wxHost()  {
	return wxHost_;
      }

      gcp::util::ArrayRegMapDataFrameManager& getArrayShare();

      bool sim() {return sim_;}
      bool simRx() {return simRx_;}

      private:
      
      //============================================================
      // Master private members & methods
      //============================================================
      
      bool sim_;
      bool simRx_;

      /**
       * A private pointer to ourselves for use inside static
       * functions.
       */
      static Master* master_;
      
      /**
       * The shared memory object will be instantiated by the master
       */
      gcp::util::ArrayRegMapDataFrameManager arrayShare_;

      /**
       * The host to connect to.
       */
      std::string ctlHost_;
      std::string wxHost_;

      //------------------------------------------------------------
      // Thread management methods
      //------------------------------------------------------------
      
      /**
       * Startup routines for the subsystem threads
       */
      static THREAD_START(startControl); 
      static THREAD_START(startScanner); 
      static THREAD_START(startSignal);  
      
      /**
       * Cleanup routines for the subsystem threads
       */
      static THREAD_CLEAN(cleanControl); 
      static THREAD_CLEAN(cleanScanner); 
      static THREAD_CLEAN(cleanSignal);  
      
      /**
       * Ping routines for the subsystem threads
       */
      static THREAD_PING(pingControl); 
      static THREAD_PING(pingScanner); 
      
      //------------------------------------------------------------
      // The thread startup functions will instantiate these objects
      //------------------------------------------------------------
      
      /**
       * Translates control commands from the ACC into CORBA calls
       * to the AC.
       */
      Control* control_;
      
      /**
       * Receives events from the AC into data frames and
       * packs them for transmission to the ACC.
       */
      Scanner* scanner_; 
      
      //------------------------------------------------------------
      // Static functions which will be passed to the
      // Signal class as callbacks on receipt of signals.
      //------------------------------------------------------------
      
      /**
       * A signal-callback method to send a message to initiate a
       * heartbeat-response cycle.
       */
      static SIGNALTASK_HANDLER_FN(sendSendHeartBeatMsg);
      
      /**
       * A message to restart system threads.
       */
      static SIGNALTASK_HANDLER_FN(sendSendRestartMsg);
      
      /**
       * A message to shutdown.
       */
      static SIGNALTASK_HANDLER_FN(sendShutDownMsg);
      
      /**
       * Trap segmentation fault signal
       */
      static SIGNALTASK_HANDLER_FN(trapSegv);

      /**
       * Send a message to the Scanner task to start a new data frame.
       */
      static SIGNALTASK_HANDLER_FN(sendStartDataFrameMsg);
      
      /**
       * Send a message to the Scanner task to connect to the archiver port.
       */
      static SIGNALTASK_HANDLER_FN(sendScannerConnectMsg);
      
      /**
       * Send a message to the Control task to connect to the
       * control port.  
       */
      static SIGNALTASK_HANDLER_FN(sendControlConnectMsg);
      
      //------------------------------------------------------------
      // Message forwarding methods for spawned tasks.
      //------------------------------------------------------------
      
      /**
       * Method by which other tasks can forward messages to the
       * control task.
       */
      MASTER_TASK_FWD_FN(forwardControlMsg);
      
      /**
       * Method by which other tasks can forward messages to the
       * scanner task.
       */
      MASTER_TASK_FWD_FN(forwardScannerMsg);
      
      //------------------------------------------------------------
      // Methods called in response to messages received on the
      // Master msgQ.
      //------------------------------------------------------------
      
      /**
       * Initiate a heartbeat request.
       */
      void sendHeartBeat();
      
      /**
       * Restart subsystem threads.
       */
      void restart();
      
      /**
       * Install signals of interest to us.
       */
      void installSignals();
      
      /**
       * Install timers of interest to us.
       */
      void installTimers();

      /**
       * A method to process a message received on our message
       * queue.  Forwards messages for other tasks via the task
       * forwarding methods above, and calls processMasterMsg()
       * below for messages intended for the master.
       */
      void processMsg(MasterMsg* taskMsg);
      
    }; // End class Master
    
} // End namespace mediator
} // End namespace gcp
  
#endif // End #ifndef 
