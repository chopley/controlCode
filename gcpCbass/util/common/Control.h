#ifndef GCP_UTIL_CONTROL_H
#define GCP_UTIL_CONTROL_H

/**
 * @file Control.h
 * 
 * Tagged: Tue Mar 23 06:23:03 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/FdSet.h"
#include "gcp/util/common/NetStr.h"
#include "gcp/util/common/SignalTask.h"
#include "gcp/util/common/Ports.h"
#include "gcp/util/common/TcpClient.h"
#include "gcp/util/common/XtermManip.h"

namespace gcp {
  namespace util {
    
    class Control {
    public:
      
      /**
       * Constructor.
       */
      Control(std::string host, bool log);
      
      /**
       * Run this object.
       */
      void processCommands();
      void processCommands2();
      void processAll();
      void processMessages();
      void readMessage();
      void readStdin();
      
      /**
       * Destructor.
       */
      virtual ~Control();
      
    private:
      
      XtermManip xterm_;
      bool doEsc_;
      std::ostringstream os_, esc_;

      // A static pointer to ourselves for use in handlers
      
      static Control* control_;
      
      // An object for handling signals
      
      SignalTask signalTask_;
      
      // A client object for managing our connection to the control program
      
      TcpClient client_;
      
      // A network buffer for communication with the control program
      
      NetStr netStr_;
      
      // A set of file descriptors
      
      FdSet fdSet_;
      
      // True when we should exit.
      
      bool stop_;
      
      // Thread management objects
      
      Thread* readThread_;
      
      static THREAD_START(startRead);
      
      Thread* sendThread_;
      
      static THREAD_START(startSend);
      
      /**
       * Read a command from stdin
       */
      void readCommand();
      
      /**
       * Send a command to the control program
       */
      void sendCommand();
      
      /**
       * A handler to be called to exit
       */
      static SIGNALTASK_HANDLER_FN(exitHandler);
      
      /**
       * Method called when a command has been sent.
       */
      static NET_SEND_HANDLER(sendHandler);
      
      /**
       * Method called when a command has been sent.
       */
      static NET_READ_HANDLER(readHandler);
      
      /**
       * Method called when an error occurs communicating with the
       * control program
       */
      static NET_SEND_HANDLER(errorHandler);

      void setRawMode(int fd);
      
    }; // End class Control
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_CONTROL_H
