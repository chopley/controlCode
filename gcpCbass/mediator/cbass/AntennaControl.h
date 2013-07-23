#ifndef GCP_MEDIATOR_ANTENNACONTROL_H
#define GCP_MEDIATOR_ANTENNACONTROL_H

/**
 * @file AntennaControl.h
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
#include "gcp/util/common/TimeVal.h"

#include "gcp/mediator/specific/ControlMsg.h"

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
    class AntennaControl :
      public gcp::util::GenericTask<AntennaControlMsg> {
      
      public:
      
      /**
       * Constructor.
       */
      AntennaControl(Control* parent);
      
      /**
       * Destructor.
       */
      virtual ~AntennaControl();
      
      private:
      
      /**
       * Control will call our sendTaskMsg() method.
       */
      friend class Control;

      /**
       * This task's parent
       */
      Control* parent_;

      /**
       * A static pointer for use in static methods.
       */
      static AntennaControl* control_;

      /**
       * The number of antennas we know about.
       */
      int nAntenna_;
      
      /**
       * The server socket on which to listen for connection
       * requests from the antenna computers.
       */
      gcp::util::TcpListener* listener_;
      
      /**
       * A network buffers associated with antennas whose connections
       * we have accepted, but for which a greeting-response cycle is
       * not yet complete.
       */
      gcp::util::NetCommHandler temporaryHandler_; 
      
      /**
       * A vector of network buffers associated with established
       * antenna connections.
       */
      std::vector<gcp::util::NetCommHandler*> connectedHandlers_; 

      /**
       * A list of initialization commands.
       */
      std::list<AntennaControlMsg> initScript_;

      /**
       * A pointer to the current element of the list
       */
      std::list<AntennaControlMsg>::iterator initScriptIter_;

      /**
       * True when we are recording an initialization script.
       */
      bool recordingInitScript_;

      /**
       * True when an initialization is in progress.
       */
      bool initInProgress_;

      /**
       * A type for specifying the initialization state of an antenna
       */
      enum InitState {
	UNINITIALIZED,
	INITIALIZING,
	INITIALIZED
      };

      /**
       * A vector of intialization flags for antennas.
       */
      std::vector<InitState> antennaInitState_;
      
      enum ConnectState {
	DISCONNECTED,
	PENDING,
	CONNECTED
      };

      /**
       * A vector of connected flags for antennas
       */
      std::vector<ConnectState> antennaConnectState_;

      /**
       * A vector of pending flags for each antenna
       */
      std::vector<bool> sendPending_;

      gcp::util::LogMsgHandler logMsgHandler_;

      /**
       * Decrement/Increment the count of commands pending to antennas.
       */
      void setPending(unsigned iant, bool pending);

      /**
       * The number of antennas currently connected.
       */
      int nConnected_;
      
      /**
       * Time structs we will use for handling timeouts in select.
       */
      gcp::util::TimeVal startTime_;
      gcp::util::TimeVal timer_;
      struct timeval* timeOut_;

      /**
       * A count of pending network commands to antennas
       */
      unsigned int pending_;

      /**
       * Attempt to get a control connection to a single antenna
       */
      bool haveControlConnection(unsigned short iant);
      
      /**
       * Set up for TCP/IP communications
       */
      void connectTcpIp();
      
      /**
       * Process a message received on our message queue
       *
       * @throws Exception
       */
      void processMsg(AntennaControlMsg* taskMsg);
      
      /**
       * Process an AntennaDrive message
       *
       * @throws Exception
       */
      void processAntennaDriveMsg(AntennaControlMsg* taskMsg);
      
      /**
       * Process a Tracker message
       *
       * @throws Exception
       */
      void processTrackerMsg(AntennaControlMsg* taskMsg);
      
      /**
       * Respond to a message to send a network command to the antenna
       * control socket.
       */
      void sendRtcNetCmd(AntennaControlMsg* msg);

      /**
       * Decrement the count of commands pending to antennas.
       */
      void decrementPending();

      /**
       * A handler to be called when a message is sent to an antenna.
       */
      static NET_SEND_HANDLER(netMsgSentHandler);

      /**
       * A handler to be called when a command is sent to an antenna.
       */
      static NET_SEND_HANDLER(netCmdSentHandler);
   
      /**
       * A handler to be called when an error occurs sending a command to an
       * antenna.
       */
      static NET_ERROR_HANDLER(netErrorHandler);

      //------------------------------------------------------------
      // Methods to do with antenna initialization
      //------------------------------------------------------------
   
      /**
       * Set the initialization state of an antenna
       */
      void setInitState(InitState state);

      /**
       * Record the next command in an initialization script
       */
      void recordInitMsg(AntennaControlMsg* msg);

      /**
       * Start an initialization script.
       */
      void beginInitScript();

      /**
       * Send the next initialization script command
       */
      void sendNextInitMsg();
	
      /**
       * Finish an initialization script.
       */
      void endInitScript();

      /**
       * Send a request for an udpate of ephemeris data from the ACC.
       */
      void sendNavUpdateMsg();

      /**
       * Return true if any antennas are ready to be initialized
       */
      bool haveAntennasReady();
  
      /**
       * Respond to a message that an antenna changed state
       */
      void flagAntenna(AntennaControlMsg* msg);

      /**
       * Return true if we are currently listening for connection
       * requests.
       */
      bool listening();
      
      /**
       * Overwrite the base-class event loop
       */
      void serviceMsgQ();
      
      /**
       * Initialize a connection to an antenna.
       */
      void initializeConnection();

      /**
       * Finalize a connection to an antenna.
       */
      void finalizeConnection(); 

      /**
       * Terminate a socket connection
       */
      void terminateConnection(gcp::util::NetCommHandler* str, bool shutdown=true);
      
      /**
       * Terminate a connection to an antenna
       */
      void terminateConnection(unsigned short iant, bool shutdown=true);

      /**
       * Buffer a net log message intended for the control task.
       */
      void bufferLogMsg(gcp::util::NetMsg* msg);

      // Forward a net message to the control task

      void forwardNetMsg(gcp::util::NetMsg* msg);

      /**
       * Send a greeting message to an antenna
       */
      void sendGreetingMsg(gcp::util::NetCommHandler* netHandler);

      /**
       * Act on receipt of a net message.
       */
      static NET_READ_HANDLER(processTemporaryNetMsg);
      static NET_READ_HANDLER(processNetMsg);

      /**
       * Start listening for connections from antennas.
       */
      void listen(bool restartListening);

      /**
       * Return true if we timed out waiting for a response from an antenna
       */
      bool timedOut();

      /**
       * Mark subsequent commands as belonging to the initialization
       * script.
       */
      void startRecordingInitScript();

      /**
       * Finish recording initialization commands
       */
      void stopRecordingInitScript();

    }; // End class AntennaControl
    
} // End namespace mediator
} // End namespace gcp
  
#endif // End #ifndef 
  
  
