#ifndef GCPP_UTIL_HORIZONSCOMMUNICATOR_H
#define GCPP_UTIL_HORIZONSCOMMUNICATOR_H

/**
 * @file HorizonsCommunicator.h
 * 
 * Tagged: Mon Jul 19 14:47:35 PDT 2004
 * 
 * @author Erik Leitch
 */
#include <list>
#include <map>
#include <string>
#include <sstream>
#include <fstream>

#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/Date.h"
#include "gcp/util/common/Communicator.h"
#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/GenericTaskMsg.h"
#include "gcp/util/common/SpawnableTask.h"
#include "gcp/util/common/String.h"
#include "gcp/util/common/TcpClient.h"
#include "gcp/util/common/TimeOut.h"

#include "gcp/control/code/unix/libunix_src/common/genericregs.h"

#define HORIZONS_FILENAME_MAX 100
#define HORIZONS_HANDLER(fn) void (fn)(void* args, std::string srcName, std::string fileName, bool error)

namespace gcp {
  namespace util {
    
    class SshTunnel;

    //------------------------------------------------------------ 
    // A utility class for sending messages to the
    // HorizonsCommunicator task
    //------------------------------------------------------------

    class HorizonsCommunicatorMsg : public GenericTaskMsg {
    public:

      enum MsgType {
        GET_EPHEM,
	CHECK_EPHEM,
	ADD_HANDLER,
	REM_HANDLER
      };

      union {

        struct {
          char   fileName[HORIZONS_FILENAME_MAX];
          char   sourceId[SRC_LEN];
          double mjdStart;
          double mjdStop;
        } getEphem;

	struct {
	  HORIZONS_HANDLER(*fn);
	  void* args;
	} addHandler;

	struct {
	  HORIZONS_HANDLER(*fn);
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
      HORIZONS_HANDLER(*fn_);
      void* args_;
    };

    //-----------------------------------------------------------------------
    // Main class definition
    //-----------------------------------------------------------------------

    class HorizonsCommunicator : public Communicator,
      public SpawnableTask<HorizonsCommunicatorMsg> {
    public:

      // An enumerated type corresponding to the current state of the
      // state machine

      enum CommState {
	STATE_INIT,  // Initial state
	STATE_WAIT,  // We are waiting to check if an ephemeris needs
		     // updating
	STATE_COMM,  // We are waiting for a response from the server
	STATE_CHECK, // We are communicating with the server during an
		     // automated check of source ephemerides
      };

      // Constructor.

      HorizonsCommunicator(unsigned intervalInSeconds=300, 
			   bool useSshTunnel=false,
			   std::string gateway="");
			   

      HorizonsCommunicator(FdSet* fdSetPtr, 
			   unsigned intervalInSeconds=300, 
			   bool useSshTunnel=false,
			   std::string getway="");

      // Destructor.

      virtual ~HorizonsCommunicator();
      
      //------------------------------------------------------------
      // Public control methods for this object
      //------------------------------------------------------------

      // Fetch an ephemeris from the horizons server

      void getEphem(std::string source,
		    std::string fileName,
                    Date start, Date stop);

      // Add a callback function to be called whenever the ephemeris
      // file is updated from the USNO server

      void addHandler(HORIZONS_HANDLER(*handler), void* args=0);

      // Remove a callback function from the list to be called

      void removeHandler(HORIZONS_HANDLER(*handler));

      // Register a source to be watched for ephemeris updates
      
      void registerEphemeris(std::string sourceName, std::string fileName);

      // Deregister a source to be watched for ephemeris updates
      
      void deregisterEphemeris(std::string sourceName);

      // Clear all ephemerides to be watched

      void clearEphemeris();

      void loadFile(std::string name);

    private:
      
      //------------------------------------------------------------
      // Private variables pertaining to our connection with the server
      //------------------------------------------------------------

      static const std::string horizonsHost_;
      static const unsigned horizonsPort_;
      bool useSshTunnel_;
      std::string gateway_;
      SshTunnel* tunnel_;

      CommState state_;

      //------------------------------------------------------------
      // Variables for managing a list of sources to be checked for
      // ephemeris updates
      //------------------------------------------------------------

      // The map of sources registered with this object

      std::map<std::string, std::string> sourceMap_;

      // The map of sources that need ephemeris updates

      std::map<std::string, std::string> updateMap_;

      // Iterator to the current source needing an update

      std::map<std::string, std::string>::iterator updateMapIter_;

      // A mutex for protecting access to sourceMap_

      Mutex mapGuard_;

      //------------------------------------------------------------
      // Internal variables pertaining to the current ephemeris being
      // fetched
      //------------------------------------------------------------

      std::string fileName_; // The output filename
      std::string srcName_;  // The source name
      std::string horizonsSrcName_; // The HORIZONS name for this source
      std::string interval_; // The ephemeris interval for this source
      std::string startUtc_; // The start UTC of the ephemeris being fetched
      std::string stopUtc_;  // The start UTC of the ephemeris being fetched

      //------------------------------------------------------------
      // Const variable of this class
      //------------------------------------------------------------

      std::map<std::string, std::string> horizonsSrcNames_;
      std::map<std::string, std::string> intervals_;

      // How long before the current ephemeris expires do we start
      // trying to get a new one?

      double expiryThresholdInDays_;  

      // How old can the current ephemeris file be before we start
      // checking for a new one?

      double updateThresholdInDays_;

      //------------------------------------------------------------
      // Variables pertaining to communications with the server
      //------------------------------------------------------------

      FdSet* fdSetPtr_;

      TimeOut retryTimeOut_;       // A timeout on which we will check the ephemeris file
      TimeOut initialTimeOut_;     // An initial timeout on startup, before checking
      TimeOut commTimeOut_;        // A timeout while communicating with the FTP server
      struct timeval* timeOutPtr_; // A pointer to one of the above

      // A list of handlers to be called when the ephemeris is updated

      std::vector<Handler> handlers_; 

      //-----------------------------------------------------------------------
      // Generic methods
      //-----------------------------------------------------------------------

      // Initialize pertinent members of this class to sensible
      // defaults

      void initialize(bool useSshTunnel, std::string gateway);

      // Initialize the various timeouts used by this class

      void setupTimeOuts(unsigned timeOutIntervalInSeconds);

      // Advance to the requested state

      void stepState(CommState state);

      //-----------------------------------------------------------------------
      // Private methods used in our select() loop
      //-----------------------------------------------------------------------
      
      void serviceMsgQ();
      void processMsg(HorizonsCommunicatorMsg* msg);

      // React to a timeoutu in select

      void registerTimeOut();

      // React to a failure of the server to reply during a
      // communications session with the server

      void registerCommTimeOut();

      // Check connection to the gateway or server, and establish an
      // ssh tunnel if necessary

      bool initiateSshConnection();

      // Terminate any ssh tunnel that may have been initiated

      void terminateSshConnection();

      // Terminate a command sequence to the horizons server

      void terminateCommSequence(bool error);
      
      //-----------------------------------------------------------------------
      // Methods called in response to messages received on our message queue
      //-----------------------------------------------------------------------

      void executeAddHandler(HORIZONS_HANDLER(*handler), void* args=0);
      void executeRemoveHandler(HORIZONS_HANDLER(*handler));

      //-----------------------------------------------------------------------
      // Methods used in communicating with the server
      //-----------------------------------------------------------------------

      // Initiate sending commands to the horizons server

      void initiateGetEphemerisCommSequence(std::string body, 
					    std::string fileName,
					    std::string startUtc, 
					    std::string stopUtc);
      
      // Compile the state machine we will use for communicating with
      // the horizons

      void compileGetEphemerisStateMachine(std::string& body, 
					   std::string& startUtc, 
					   std::string& stopUtc);
 
      // Callback function while reading ephemeris lines from the
      // server

      static COMM_PARSER_FN(readEphemeris);
      void readEphemeris(gcp::util::String& str);

      // Callback function when the server is done sending ephemeris
      // lines

      static COMM_END_PARSER_FN(endEphemeris);
      void endEphemeris();

      // Write a newly received ephemeris to a file

      void writeEphem();
      void printHeader(std::ofstream& fout);
      void printEphem(std::ofstream& fout);

      // Callback to disconnect from the server when we are done
      // comomunicating with it

      static COMM_PARSER_FN(quitFromServer);
      void quitFromServer();

      // Called to initiate communications with the server to get the
      // ephemeris for the next source

      void checkNextSource();

      // Call any handler that were registered on succesful read of
      // an ephemeris

      void callHandlers(bool error);

      //------------------------------------------------------------
      // Private methods to set/access internal variables
      //------------------------------------------------------------

      public:
      void setFilename(std::string fileName);
      void testFn();

      private:
      void setSource(std::string src);

      void setEphemerisDates();

      std::string getSource();
      std::string getFilename();
      std::string getHorizonsSource();
      std::string getInterval();

      //------------------------------------------------------------
      // Ephemeris checking
      //------------------------------------------------------------

      // Return true if any registered ephemeris file needs updating

      bool ephemerisNeedsUpdating();
      
      // Return true if the ephemeris file exists

      bool ephemExists(std::string ephemFileName);

      // Return true if the ephemeris is with expiryThresholdInDays_
      // of running out

      bool ephemIsAboutToRunOut(std::string ephemFileName); 
   
      // Return the MJD of the last entry in the ephemeris file

      double getLastEphemMjd(std::string ephemFileName);

      // Return true if the ephemeris hasn't been updated in more than
      // updateThresholdInDays_

      bool ephemFileIsOutOfDate(std::string ephemFileName);

    }; // End class HorizonsCommunicator
    
  } // End namespace UTIL
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_HORIZONSCOMMUNICATOR_H
