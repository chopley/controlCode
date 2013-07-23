#ifndef GCP_UTIL_TIPPERCOMMUNICATOR_H
#define GCP_UTIL_TIPPERCOMMUNICATOR_H

/**
 * @file TipperCommunicator.h
 * 
 * Tagged: Mon Jul 19 14:47:35 PDT 2004
 * 
 * @author Erik Leitch
 */
#include <list>
#include <string>
#include <sstream>

#include "gcp/util/common/Communicator.h"
#include "gcp/util/common/FdSet.h"
#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/GenericTaskMsg.h"
#include "gcp/util/common/SpawnableTask.h"
#include "gcp/util/common/String.h"
#include "gcp/util/common/TcpClient.h"
#include "gcp/util/common/TimeOut.h"

namespace gcp {
  namespace util {
    
    // A utility class for sending messages to the ModemPager task

    class TipperCommunicatorMsg : public GenericTaskMsg {
    public:

      enum MsgType {
	GET_FILE,
      };
      
      union {
	bool enable;
      } body;
      
      // A type for this message

      MsgType type;
    };

    class TipperCommunicator : public Communicator,
      public SpawnableTask<TipperCommunicatorMsg> {
    public:

      //-----------------------------------------------------------------------
      // Methods for use of this object from an external thread
      //-----------------------------------------------------------------------

      // Constructor for external use of this class (ie, run from
      // another thread)

      TipperCommunicator(gcp::util::FdSet* fdSet, std::string host);

      // Return the fd corresponding to the communications connection
      // to the FTP server

      int getFtpCommFd();

      // Return the fd corresponding to the data connection to the FTP
      // server

      int getFtpDataFd();

      //-----------------------------------------------------------------------
      // Methods for use of this object in a stand-alone thread
      //-----------------------------------------------------------------------

      // Constructor for internal use of this class (ie, run in its
      // own thread).  If timeOutIntervalInSeconds is non-zero, this
      // object will automatically retrieve the tipper log on the
      // specified interval.  Else it will do nothing until told to
      // retrieve it.

      TipperCommunicator(std::string host, unsigned timeOutIntervalInSeconds=0);
      
      // Tell this object to retrieve the tipper log

      void getTipperLog();

      //-----------------------------------------------------------------------
      // Generic methods
      //-----------------------------------------------------------------------

      // Initialize pertinent members of this class to sensible
      // defaults

      void initialize(std::string host);

      // Destructor.

      virtual ~TipperCommunicator();

      // Initiate the comms sequence to retrieve the tipper log from
      // the remote server

      void initiateGetTipperLogCommSequence();

      // Read a line from the ftp server and determine what to do

      void concatenateString(std::ostringstream& os);

      // Process a tipper log received from the remote FTP server

      void processTipperLog();
  
    private:
      
      // A client for communicating wth the remote FTP server

      gcp::util::TcpClient* ftpClient_;

      // A pointer to an FdSet object, possibly external

      gcp::util::FdSet* fdSetPtr_;

      // A timeout on which we will retrieve the tipper log from the
      // remote FTP server

      TimeOut timeOut_;

      //-----------------------------------------------------------------------
      // Methods for use of this object in a stand-alone thread
      //-----------------------------------------------------------------------

      void serviceMsgQ();
      void processMsg(TipperCommunicatorMsg* msg);

      //-----------------------------------------------------------------------
      // Generic methods
      //-----------------------------------------------------------------------

      static COMM_PARSER_FN(parsePortNumber);
      void parsePortNumber();

      static COMM_PARSER_FN(quitFromServer);
      void quitFromServer();

      // React to a failure to reply

      void registerTimeOut();

      // Terminate a command sequence to the

      void terminateCommSequence(bool error);
      
      // Compile the list of communication/responses needed for
      // retrieving the tipper log from the remote FTP server

      void compileGetTipperLogStateMachine();
 
    }; // End class TipperCommunicator
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_TIPPERCOMMUNICATOR_H
