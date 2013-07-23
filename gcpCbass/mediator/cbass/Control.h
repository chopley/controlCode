#ifndef GCP_MEDIATOR_CONTROL_H
#define GCP_MEDIATOR_CONTROL_H

/**
 * @file Control.h
 * 
 * Tagged: Thu Nov 13 16:53:57 UTC 2003
 * 
 * @author Erik Leitch
 */
#include <list>
#include <vector>

// Shared control code includes

#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"

// SZA antenna code includes

#include "gcp/util/common/ArrayRegMapDataFrameManager.h"
#include "gcp/util/common/FdSet.h"
#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/Logger.h"
#include "gcp/util/common/LogMsgHandler.h"
#include "gcp/util/common/NetCommHandler.h"
#include "gcp/util/common/TcpClient.h"

#include "gcp/mediator/specific/ControlMsg.h"
#include "gcp/mediator/specific/TransNetCmdForwarder.h"

#define CONTROL_TASK_FWD_FN(fn) void (fn)(ControlMsg* msg)

namespace gcp {
  namespace mediator {
    
    /**
     * Forward declaration of Master lets us use it
     * without defining it.
     */
    class Master;
    class MasterMsg;
    class Control;
    class AntennaControl;
    class NetCmdToMsg;
    class GrabberControl;
    class ReceiverControl;
    class WxControl;
    
    /**
     * A class to encapsulate the control connection between the AC
     * and the ACC.
     */
    class Control : 
      public gcp::util::GenericTask<ControlMsg> {
      
      public:
      
      /**
       * Destructor.
       */
      ~Control();
      
      //------------------------------------------------------------
      // Task message forwarding methods
      //------------------------------------------------------------
      
      /**
       * Method by whic5Bh other tasks can forward messages to the
       * AntennaControl task.
       */
      CONTROL_TASK_FWD_FN(forwardAntennaControlMsg);
      
      /**
       * Method by which other tasks can forward messages to us.
       */
      CONTROL_TASK_FWD_FN(forwardControlMsg);
      
      /**
       * Method by which other tasks can forward messages to the
       * GrabberControl task.
       */
      CONTROL_TASK_FWD_FN(forwardGrabberControlMsg);
      
      /**
       * Method by which other tasks can forward messages to the
       * ReceiverControl task.
       */
      CONTROL_TASK_FWD_FN(forwardReceiverControlMsg);

      /**
       * Method by which other tasks can forward messages to the
       * WxControl task.
       */
      CONTROL_TASK_FWD_FN(forwardWxControlMsg);
      
      /**
       * Method by which other tasks can forward messages to the
       * Master task.
       */
      void forwardMasterMsg(MasterMsg* msg);
      
      void forwardNetCmd(gcp::util::NetCmd* netCmd);
      
      gcp::util::RegMapDataFrameManager* getArrayShare();
      
      Master* parent() {return parent_;}

      std::string wxHost();

      private:
      
      /**
       *  will access private members of this class.
       */
      friend class Master;
      friend class AntennaControl;
      friend class GrabberControl;
      friend class ReceiverControl;
      friend class WxControl;
      
      /**
       * A static pointer for use in static handlers.
       */
      static Control* control_;
      
      /**
       * A network stream handler for handling communication with the
       * control program
       */
      gcp::util::NetCommHandler netCommHandler_;
      
      /**
       * And object for forwarding network commands.
       */
      TransNetCmdForwarder* forwarder_;
      
      /**
       * An object for managing net messages
       */
      gcp::util::LogMsgHandler logMsgHandler_;

      //------------------------------------------------------------
      // Declare startup functions for threads managed by this class.
      //------------------------------------------------------------
      
      /**
       * The antenna control task.
       */
      static THREAD_START(startAntennaControl);
      
      /**
       * The grabber control task.
       */
      static THREAD_START(startGrabberControl);
      
      /**
       * The receiver control task.
       */
      static THREAD_START(startReceiverControl);

      /**
       * The wx control task.
       */
      static THREAD_START(startWxControl);
      
      //------------------------------------------------------------
      // Declare cleanup handlers for threads managed by this class.
      //------------------------------------------------------------
      
      /**
       * The antenna control task.
       */
      static THREAD_CLEAN(cleanAntennaControl);
      
      /**
       * The frame grabber control task.
       */
      static THREAD_CLEAN(cleanGrabberControl);
      
      /**
       * The frame receiver control task.
       */
      static THREAD_CLEAN(cleanReceiverControl);

      /**
       * The frame wx control task.
       */
      static THREAD_CLEAN(cleanWxControl);
      
      //------------------------------------------------------------
      // Declare methods by which we can ping spawned threads
      //------------------------------------------------------------
      
      /**
       * The antenna control task.
       */
      static THREAD_PING(pingAntennaControl);
      
      /**
       * The grabber control task.
       */
      static THREAD_PING(pingGrabberControl);
      
      /**
       * The receiver control task.
       */
      static THREAD_PING(pingReceiverControl);

      /**
       * The wx control task.
       */
      static THREAD_PING(pingWxControl);
      
      /**
       * A private pointer to the parent class
       */
      Master* parent_;

      /**
       * Resources of threads spawned by this task.
       */      
      AntennaControl*  antennaControl_;
      GrabberControl*  grabberControl_;
      ReceiverControl* receiverControl_;
      WxControl*       wxControl_;
      
      /**
       * An object for managing our connection to the host machine
       */
      gcp::util::TcpClient client_;
      
      /**
       * The IP address of the control host
       */
      std::string host_;        
      
      /**
       * true if connected to the control port
       */
      bool connected_;     
      
      /**
       * Private constructor insures that Control can only be
       * instantiated by Master.
       */
      Control(Master* master);
      
      /**
       * True when connected to the RTC control port
       */
      bool isConnected(); 
      
      /**
       * Connect to the control socket
       */
      bool connect();    
      
      /**
       * Disconnect from the control socket
       */
      void disconnect(); 
      
      /**
       * Attempt to connect to the host
       */
      void connectControl(bool reEnable);
      
      /**
       * Disconnect from the host.
       */
      void disconnectControl();
      
      /**
       * Service our message queue.
       */
      void serviceMsgQ();
      
      /**
       * Send a heartbeat request to all threads mananged by this task.
       */
      void sendHeartBeat();
      
      /**
       * A method to send a connection status message to the master.
       */
      void sendControlConnectedMsg(bool connected);
      
      /**
       * A method to send a receiver script completion message
       */
      void sendScriptDoneMsg(unsigned seq);
      
      /**
       * A method to send the antenna control task an init message.
       */
      void sendAntennaInitMsg(bool start);
      
      /**
       * Send a string to be logged back to the control program.
       */
      static void sendNetLogMsg(std::string logStr, bool isErr);

      /**
       * Send a string to be logged back to the control program.
       */
      static LOG_HANDLER_FN(sendLogMsg);
      
      /**
       * Send an error string to be logged back to the control
       * program.
       */
      static LOG_HANDLER_FN(sendErrMsg);
      
      /**
       * Process a message received on our message queue
       *
       * @throws Exception
       */
      void processMsg(ControlMsg* taskMsg);
      
      /**
       * Override GenericTask::respondToHeartBeat()
       */
      void respondToHeartBeat();
      
      /**
       * Pack a network message intended for the ACC
       */
      void packNetMsg(ControlMsg* msg);
      
      /**
       * Read a command from the control program.
       */
      static NET_READ_HANDLER(readNetCmdHandler);
      
      /**
       * Send a message to the control program.
       */
      static NET_SEND_HANDLER(sendNetMsgHandler);
      
      /**
       * Call this function if an error occurs while communicating
       * with the ACC
       */
      static NET_ERROR_HANDLER(networkErrorHandler);
      
    }; // End class Control
    
  } // End namespace mediator
} // End namespace gcp

#endif // End #ifndef 
