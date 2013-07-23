#ifndef GCP_UTIL_IERSCOMMUNICATOR_H
#define GCP_UTIL_IERSCOMMUNICATOR_H

/**
 * @file IersCommunicator.h
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

#define IERS_HANDLER(fn) void (fn)(void* args, std::string fileName)
#define IERS_FILENAME_MAX 100

namespace gcp {
  namespace util {
    
    //------------------------------------------------------------
    // A utility class for sending messages to the ModemPager task
    //------------------------------------------------------------

    class IersCommunicatorMsg : public GenericTaskMsg {
    public:

      enum MsgType {
	GET_FILE,
	UPDATE_FILENAME,
	ADD_HANDLER,
	REM_HANDLER
      };
      
      union {

	bool enable;

	char fileName[IERS_FILENAME_MAX+1];

	struct {
	  IERS_HANDLER(*fn);
	  void* args;
	} addHandler;

	struct {
	  IERS_HANDLER(*fn);
	} remHandler;

      } body;
      
      // A type for this message

      MsgType type;
    };

    //------------------------------------------------------------
    // A utility class used to store handlers
    //------------------------------------------------------------

    class Handler {
    public:
      IERS_HANDLER(*fn_);
      void* args_;
    };

    //------------------------------------------------------------
    // Main class definition
    //------------------------------------------------------------

    class IersCommunicator : public Communicator,
      public SpawnableTask<IersCommunicatorMsg> {
    public:

      // An enumerated type corresponding to the current state of the
      // state machine

      enum CommState {
	STATE_INIT, // Initial state
	STATE_WAIT, // We are waiting to check if an ephemeris needs updating
	STATE_COMM, // We are waiting for a response from the ftp server
      };

      //-----------------------------------------------------------------------
      // Methods for use of this object in a stand-alone thread
      //-----------------------------------------------------------------------

      // Constructors for internal use of this class (ie, run in its
      // own thread). If timeOutIntervalInSeconds is non-zero, this
      // object will automatically retrieve the UT1-UTC ephemeris on
      // the specified interval.  Else it will do nothing until told
      // to retrieve it.

      IersCommunicator(std::string fullPathToOutputFile,
		       unsigned timeOutIntervalInSeconds=0);

      IersCommunicator(std::string outputDir,
		       std::string outputFileName,
		       unsigned timeOutIntervalInSeconds=0);
      
      // Destructor.

      virtual ~IersCommunicator();

      //------------------------------------------------------------
      // Public control methods for this object
      //------------------------------------------------------------

      // Tell this object to retrieve the IERS bulletin

      void getIersBulletin();

      // Update the ephemeris file that this object will check

      void updateEphemerisFileName(std::string fileName);

      // Add a callback function to be called whenever the ephemeris
      // file is updated from the USNO server

      void addHandler(IERS_HANDLER(*handler), void* args=0);

      // Remove a callback function from the list to be called

      void removeHandler(IERS_HANDLER(*handler));

      //------------------------------------------------------------
      // Query functions
      //------------------------------------------------------------

      // Get the ephemeris file name

      std::string ephemFileName();

      //------------------------------------------------------------
      // Test functions
      //------------------------------------------------------------

      // Load a ser7.dat file to be converted

      void loadFile(std::string name);

    private:
      
      // The current state of the state machine

      CommState state_;               

      // A list of handlers to be called when the ephemeris is updated

      std::vector<Handler> handlers_; 

      // How long before the current ephemeris expires do we start
      // trying to get a new one?

      double expiryThresholdInDays_;  

      // How old can the current ephemeris file be before we start
      // checking for a new one?

      double updateThresholdInDays_;

      // An output stream used to store responses from the FTP server

      std::ostringstream ftpOs_;

      // The name of the file containing the ut1-utc ephemeris

      std::string outputFileName_;

      // A client for communicating wth the remote FTP server

      gcp::util::TcpClient* ftpClient_;

      // A pointer to an FdSet object, possibly external

      gcp::util::FdSet* fdSetPtr_;

      // Timeouts used by this class

      TimeOut retryTimeOut_;       // A timeout on which we will check the ephemeris file
      TimeOut initialTimeOut_;     // An initial timeout on startup, before checking
      TimeOut commTimeOut_;        // A timeout while communicating with the FTP server
      struct timeval* timeOutPtr_; // A pointer to one of the above

      //-----------------------------------------------------------------------
      // Generic methods
      //-----------------------------------------------------------------------

      // Initialize pertinent members of this class to sensible
      // defaults

      void initialize(std::string fullPathToOutputFile);
      void initialize(std::string outputDir, std::string outputFileName);

      // Initialize the various timeouts used by this class

      void setupTimeOuts(unsigned timeOutIntervalInSeconds);

      // Advance to the requested state

      void stepState(CommState state);

      //-----------------------------------------------------------------------
      // Methods called in response to messages received on our message queue
      //-----------------------------------------------------------------------

      void executeAddHandler(IERS_HANDLER(*handler), void* args=0);
      void executeRemoveHandler(IERS_HANDLER(*handler));

      //-----------------------------------------------------------------------
      // Run methods used by this class
      //-----------------------------------------------------------------------

      void serviceMsgQ();
      void processMsg(IersCommunicatorMsg* msg);

      // React to a timeout in select

      void registerTimeOut();

      // Return the fd corresponding to the communications connection
      // to the FTP server

      int getFtpCommFd();

      // Return the fd corresponding to the data connection to the FTP
      // server

      int getFtpDataFd();

      //-----------------------------------------------------------------------
      // Private methods used to communicate with the USNO server
      //-----------------------------------------------------------------------

      // Compile the list of communication/responses needed for
      // retrieving the tipper log from the remote FTP server

      void compileGetIersBulletinStateMachine();
 
      // Initiate the comms sequence to retrieve the tipper log from
      // the remote server

      void initiateGetIersBulletinCommSequence();

      // Read a line from the ftp server and determine what to do

      void concatenateString(std::ostringstream& os);

      // Process data received from the remote FTP server

      void processIersBulletin();

      // Parse a data port number received from the FTP server

      static COMM_PARSER_FN(parsePortNumber);
      void parsePortNumber();

      // Quit from the FTP server.  

      static COMM_PARSER_FN(quitFromServer);
      void quitFromServer(bool error);

      // Terminate a connection with the FTP server

      void terminateFtpConnection(bool error);

      // React to a failure to reply from the FTP server once a
      // connection has been established

      void registerCommTimeOut();

      //-----------------------------------------------------------------------
      // Methods used to write a gcp-style ephemeris file
      //-----------------------------------------------------------------------

      void writeEphem();
      void printHeader(std::ofstream& fout);
      void printEphem(std::ofstream& fout);

      //-----------------------------------------------------------------------
      // Methods used to determine if the local ephemeris needs updating
      //-----------------------------------------------------------------------

      // Return true if the UT1-UTC ephemeris needs updating

      bool ephemerisNeedsUpdating();

      // Return true if it is safe to update the ephemeris file.

      bool safeToUpdate();

      // Return true if the UT1-UTC ephemeris file exists

      bool ephemExists();

      // Return true if the ephemeris is with expiryThresholdInDays_
      // of running out
      
      bool ephemIsAboutToRunOut();

      // Return the MJD of the last entry in the ephemeris file

      double getLastEphemMjd();

      // Return true if the ephemeris hasn't been updated in more than
      // updateThresholdInDays_

      bool ephemFileIsOutOfDate();

      void callHandlers();

    }; // End class IersCommunicator
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_IERSCOMMUNICATOR_H
