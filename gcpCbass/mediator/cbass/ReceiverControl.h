// $Id: ReceiverControl.h,v 1.1.1.1 2009/07/06 23:57:23 eml Exp $

#ifndef GCP_MEDIATOR_RECEIVERCONTROL_H
#define GCP_MEDIATOR_RECEIVERCONTROL_H

/**
 * @file ReceiverControl.h
 * 
 * Tagged: Fri Feb  9 22:31:51 NZDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:23 $
 * 
 * @author Erik Leitch
 */

#include <iostream>
#include <fstream>
#include <list>


#include "gcp/util/common/GenericTask.h"

#include "gcp/mediator/specific/ReceiverControlMsg.h"

namespace gcp {

  namespace util {
    class CoProc;
    class Port;
  }

  namespace mediator {

    /**    
     * Forward declaration of Control lets us use it
     * without defining it.
     */
    class Control;
    
    /**
     * A class for controlling antennas.
     */
    class ReceiverControl : 
      public gcp::util::GenericTask<ReceiverControlMsg> {
      
    public:

      
      struct Script {

	enum Status {
	  NONE,
	  FAILED,
	  OK
	};

	enum State {
	  DONE,
	  START,
	  BODY,
	  END,
	};

	enum Type {
	  ERROR,
	  MESSAGE,
	  STATUS,
	};

	// The process associated with this script

	gcp::util::CoProc* proc_;

	// A file descriptor associated with the output file for this
	// script

	std::ofstream outFile_;

	gcp::util::Port* stdOut_;
	gcp::util::Port* stdErr_;

	// Variables for handling messages received over the stderr
	// pipe

	unsigned tagCount_;

	std::ostringstream startTag_;
	std::ostringstream message_;
	std::ostringstream endTag_;

	State messageState_;
	Type messageType_;

	// The seq id number associated with this script 

	unsigned seq_;

	// A return status from this script

	Status status_;

	bool stdOutWasClosed_;
	bool stdErrWasClosed_;

	std::string script_;

	//------------------------------------------------------------
	// Methods
	//------------------------------------------------------------

	// Constructor will be called with the command name to execute

	Script(std::string script, unsigned seq, std::string dir);

	~Script();

	// Methods to return the stdout/stderr fds for this process

	int stdInFd();
	int stdOutFd();
	int stdErrFd();

	void open(std::string fileName);
	void close();
	
	// Copy data from the stdout pipe of the process to the output
	// file

	unsigned copyFromStdoutToFile();

	// Read the next string from stderr

	unsigned readFromStdErr();

      // Process a message received on stderr

	void processMessage();

	// Set the message type from the start tag that was read

	void setMessageType();

      };

      /**
       * Constructor.
       */
      ReceiverControl(Control* parent);

      /**
       * Destructor.
       */
      virtual ~ReceiverControl();

      // Forward a script message to the receiver control task.

      void sendScript(std::string script, unsigned seq);

      /**
       * Overwrite the base-class event loop
       */
      void run() {
	gcp::util::GenericTask<ReceiverControlMsg>::run();
      }

    private:

      /**
       * Control will call our sendTaskMsg() method.
       */
      friend class Control;

      Control* parent_;

      // The output directory

      std::string outputDir_;

      /**
       * The list of scripts currently running
       */
      std::list<Script*> scripts_;

      // The list of scripts that finished on the last loop of select()

      std::list<Script*> finishedScripts_;

      // A logfile object for managing output file names

      gcp::util::LogFile logFile_;
      
      /**
       * Overwrite the base-class event loop
       */
      void serviceMsgQ();
      
      /**
       * Process a message received on our message queue
       *
       * @throws Exception
       */
      void processMsg(ReceiverControlMsg* taskMsg);

      // Register receipt of a command to execute a new script

      void installNewScript(char* script, unsigned seq);
      
      // Respond to data on the stdout pipe for this script

      void processStdOut(Script* script);

      // Respond to data on the stderr pipe for this script

      void processStdErr(Script* script);

      // Respond to data on the stdout pipe for this script

      void registerStdOutClosed(Script* script);

      // Respond to data on the stdout pipe for this script
      
      void registerStdErrClosed(Script* script);

      // Respond to data on the stdout pipe for this script

      void checkCompletionStatus(Script* script);

      // Register an error installing a new script

      void registerError(std::string message, unsigned seq);

      void removeFinishedScriptsFromList();

      // Send a script completion message to the control program

      void sendScriptCompletedMsg(unsigned seq);

    }; // End class ReceiverControl

  } // End namespace mediator
} // End namespace gcp



#endif // End #ifndef GCP_MEDIATOR_RECEIVERCONTROL_H
