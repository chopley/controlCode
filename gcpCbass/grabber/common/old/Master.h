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

#include "gcp/util/common/Directives.h"
#include "gcp/util/common/GenericMasterTask.h"

#include "gcp/grabber/common/MasterMsg.h"

#define IMAGE_SIGNAL   SIGRTMIN+1
#define CONNECT_SIGNAL SIGRTMIN+2

#define CONNECT_SEC 2

#define MASTER_TASK_FWD_FN(fn) void (fn)(MasterMsg* msg)

namespace gcp {
  namespace grabber {
    
    class Control;
    class Scanner;
    
    /**............................................................  
     * A class for reading frames from the frame grabber, and sending
     * them of to the outside world.
     *
     * Instantiating a  object starts up three threads: the
     * main one, and two spawned threads, represented by the
     * following objects:
     *
     *   1) Control: which listens for network commands.
     *
     *   2) Scanner: which receives frames from the frame grabber, and
     *   sends them off to the outside world.  
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
       * @param host The control host and the host to send images to.
       */
      Master(std::string ctlHost, std::string imHost, 
	     bool simulate, unsigned channel);
      
      /**
       * Destructor.
       */
      ~Master();
      
      /**
       * Methods by which tasks can query std::strings pertinent to
       * the connection.
       */
      std::string ctlHost();
      std::string imHost();
      
      /**
       * Forward a message to the master task
       */
      MASTER_TASK_FWD_FN(forwardMasterMsg);
      
      /**
       * Forward a message to the scanner task
       */
      MASTER_TASK_FWD_FN(forwardScannerMsg);
      
      /**
       * Forward a message to the control task
       */
      MASTER_TASK_FWD_FN(forwardControlMsg);
      
      unsigned channel() {
	return channel_;
      }

    private:
      
      //============================================================
      // Master private members & methods
      //============================================================
      
      /**
       * A private pointer to ourselves for use inside static
       * functions.
       */
      static Master* master_;
      
      /**
       * Resources of threads spawned by this task.
       */      
      Control* control_;
      Scanner* scanner_;
      
      // The control host to connect to.

      std::string ctlHost_;

      // The host to send images to

      std::string imHost_;
      
      /**
       * Simulate frame grabber hardware?
       */
      bool simulate_;
      
      inline bool simulate() {
	return simulate_;
      }
      
      unsigned channel_;

      //------------------------------------------------------------
      // Thread management methods
      //------------------------------------------------------------
      
      /**
       * Startup routines for the subsystem threads
       */
      static THREAD_START(startControl);  
      
      /**
       * Cleanup routines for the subsystem threads
       */
      static THREAD_CLEAN(cleanControl);  
      
      /**
       * Startup routines for the subsystem threads
       */
      static THREAD_START(startScanner);  
      
      /**
       * Cleanup routines for the subsystem threads
       */
      static THREAD_CLEAN(cleanScanner);  
      
      /**
       * Startup routines for the subsystem threads
       */
      static THREAD_START(startSignal);  
      
      /**
       * Cleanup routines for the subsystem threads
       */
      static THREAD_CLEAN(cleanSignal);  
      
      //------------------------------------------------------------
      // Message processing
      
      /**
       * Process a message received on our message queue.
       */
      void processMsg(MasterMsg* msg);
      
      //------------------------------------------------------------
      // Static functions which will be passed to the
      // Signal class as callbacks on receipt of signals.
      //------------------------------------------------------------
      
      /**
       * A message to shutdown.
       */
      static SIGNALTASK_HANDLER_FN(sendShutDownMsg);
      
      /**
       * Send a message to connect to the control port
       */
      static SIGNALTASK_HANDLER_FN(sendConnectControlMsg);
      
      /**
       * Send a message to connect to the scanner port
       */
      static SIGNALTASK_HANDLER_FN(sendConnectScannerMsg);
      
      /**
       * Send a message indicating the state of the grabber control
       * connection.
       */
      void sendControlConnectedMsg(bool connected);
      
      /**
       * Send a message indicating the state of the grabber scanner
       * connection.
       */
      void sendScannerConnectedMsg(bool connected);
      
      /**
       * Install signals of interest to us.
       */
      void installSignals();
      
      /**
       * Install timers of interest to us.
       */
      void installTimers();
      
    }; // End class Master
    
  }; // End namespace grabber
}; // End namespace gcp


#endif // End #ifndef 
