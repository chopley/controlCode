#ifndef CONTROL_H
#define CONTROL_H

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

#include "gcp/util/common/FdSet.h"
#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/Logger.h"
#include "gcp/util/common/LogMsgHandler.h"
#include "gcp/util/common/NetCommHandler.h"
#include "gcp/util/common/TcpClient.h"

#include "gcp/grabber/common/ControlMsg.h"
#include "gcp/grabber/common/GrabberNetCmdForwarder.h"

#define CONTROL_TASK_FWD_FN(fn) void (fn)(ControlMsg* msg)

namespace gcp {
  namespace grabber {
    
    /**
     * Forward declaration of Master lets us use it
     * without defining it.
     */
    class Master;
    
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
       * Method by which other tasks can forward messages to us.
       */
      CONTROL_TASK_FWD_FN(forwardControlMsg);
      
      private:
      
      /**
       * An object for forwarding network commands 
       */
      GrabberNetCmdForwarder* forwarder_;
      
      /**
       *  Master will access private members of this class.
       */
      friend class Master;
      
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
       * A private pointer to the parent class
       */
      Master* parent_;
      
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
       * An object for managing log messages
       */
      gcp::util::LogMsgHandler logMsgHandler_;

      /**
       * Private constructor insures that Control can only be
       * instantiated by Master.
       */
      Control(Master* master);
      
      /**
       * True when connected to the grabber control port
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
       * A method to send a connection status message to the master.
       */
      void sendControlConnectedMsg(bool connected);
      
      /**
       * Send a string to be logged back to the control program.
       */
      static LOG_HANDLER_FN(sendLogMsg);
      
      /**
       * Send an error string to be logged back to the control
       * program.
       */
      static LOG_HANDLER_FN(sendErrMsg);
      
      static void sendNetMsg(std::string& logStr, bool isErr);

      /**
       * Process a message received on our message queue
       *
       * @throws Exception
       */
      void processMsg(ControlMsg* taskMsg);
      
      /**
       * Pack a network message intended for the ACC
       */
      void packNetMsg(ControlMsg* msg);
      
      /**
       * Read a command from the control program.
       */
      static NET_READ_HANDLER(netCmdReadHandler);
      
      /**
       * Send a message to the control program.
       */
      static NET_SEND_HANDLER(netMsgSentHandler);
      
      /**
       * Call this function if an error occurs while communicating
       * with the ACC
       */
      static NET_ERROR_HANDLER(netErrorHandler);
      
    }; // End class Control
    
  }; // End namespace grabber
}; // End namespace gcp


#endif // End #ifndef 
