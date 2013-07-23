#ifndef GCP_ASSEMBLER_GRABBERCONTROL_H
#define GCP_ASSEMBLER_GRABBERCONTROL_H

/**
 * @file GrabberControl.h
 * 
 * Started: Sat Jan 10 06:39:24 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/specific/Directives.h"
#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/LogMsgHandler.h"
#include "gcp/util/common/NetCommHandler.h"
#include "gcp/util/common/NetStr.h"
#include "gcp/util/common/TcpListener.h"

#include "gcp/mediator/specific/GrabberControlMsg.h"

namespace gcp {
namespace mediator {
    
    /**    
     * Forward declaration of Control lets us use it
     * without defining it.
     */
    class Control;
    
    /**
     * A class for controlling antennas.
     */
    class GrabberControl :
      public gcp::util::GenericTask<GrabberControlMsg> {
      
      public:
      
      /**
       * Constructor.
       */
      GrabberControl(Control* parent);
      
      /**
       * Destructor.
       */
      virtual ~GrabberControl();
      
      private:
      
      /**
       * Control will call our sendTaskMsg() method.
       */
      friend class Control;

      Control* parent_;
      
      /**
       * The server socket on which to listen for connection
       * requests from the antenna computers.
       */
      gcp::util::TcpListener* listener_;
      
      /**
       * A network buffer associated with the frame grabber connection
       */
      gcp::util::NetCommHandler netHandler_; 
      
      gcp::util::LogMsgHandler logMsgHandler_;

      /**
       * Process a message received on our message queue
       *
       * @throws Exception
       */
      void processMsg(GrabberControlMsg* taskMsg);
      
      /**
       * A handler to be called when a frame has been completely sent.
       */
      static NET_READ_HANDLER(netMsgReadHandler);
      
      /**
       * A handler to be called when a frame has been completely sent.
       */
      static NET_SEND_HANDLER(netCmdSentHandler);

      /**
       * A handler to be called when an error occurs in communication
       */
      static NET_ERROR_HANDLER(netErrorHandler);

      /**
       * Allow the frame grabber to connect.
       */
      void connectGrabber();
      
      /**
       * Terminate a connection to the frame grabber
       */
      void disconnectGrabber();

      /**
       * Overwrite the base-class event loop
       */
      void serviceMsgQ();
      
      /**
       * Send a network command to the frame grabber
       */
      void sendRtcNetCmd(GrabberControlMsg* msg);

      /**
       * Read a net message from the frame grabber
       */
      void readNetMsg();

      /**
       * Act upon receipt of a network message from one of our antennas.
       */
      void bufferLogMsg(gcp::util::NetMsg* msg);

      /**
       * Forward a net message received from the frame grabber to the
       * control task.
       */
      void forwardNetMsg(gcp::util::NetMsg* msg);

      /**
       * Act on receipt of a net message.
       */
      void processNetMsg();

      /**
       * Start listening for connections from antennas.
       */
      void listen(bool restartListening);

    }; // End class GrabberControl
    
} // End namespace mediator
} // End namespace gcp
  
#endif // End #ifndef 
  
  
