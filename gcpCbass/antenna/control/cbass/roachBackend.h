#ifndef ROACHBACKEND_H
#define ROACHBACKEND_H

/**
 * @file RoachBackend.h
 * 
 * Tagged: Thu Nov 13 16:53:44 UTC 2003
 * 
 * @author Stephen Muchovej
 * 
 */
// Required C header files from the array control code

#include "gcp/control/code/unix/libunix_src/common/regmap.h" // RegMapBlock

#include "gcp/antenna/control/specific/Board.h"
#include "gcp/antenna/control/specific/RoachBackendMsg.h"
#include "gcp/util/common/FdSet.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <termios.h>
#include <string>

/**
 *  LATENCY FOR ROACH BACKEND
 */
#define ROACH_TIMEOUT_USEC 5000 

namespace gcp {
  namespace antenna {
    namespace control {

      class SpecificShare;
      
      class RoachBackend : public Board {
	
      public:
	
	/**
	 * Constructor with pointer to shared resources.
	 */
	RoachBackend(SpecificShare* share, std::string name, bool sim);
	
	/**
	 * Constructor.  
	 */
	RoachBackend();
	
	/**
	 * Destructor.
	 */
	~RoachBackend();

	/**
	 * Command set -- commands called by higher functions
	 */
	void queryStatus();
	void queryStatus(gcp::util::TimeVal& currTime);

	/**
	 * Fill the utc register with the time stamps appropriate for the data
	 * just read back from the servo box
	 */
	void fillUtc(gcp::util::TimeVal& currTime);


	/**
	 * Connect to the roach
	 *
	 * Returns true on success.
	 */
	bool connect();
	
	/**
	 * Disconnect from the roach.  
	 */
	void disconnect();

	/**
	 * Return true if the roach is connected.
	 */
	bool roachIsConnected();

	/**
	 *
	 */
	sockaddr_in servaddr_;

	/**
	 * Request data from the backend
	 */
	void getData();

	
	/**
	 * Send a single command to the ROACH.
	 */
	void sendCommand(RoachBackendMsg& command);
	
	/**
	 * Read a response from the ROACH.  Does not check that a
	 * response is available, and may block waiting for one,
	 * depending on how the socket was configured.
	 */
	int readResponse(RoachBackendMsg& command);

	RoachBackendMsg command_;

	// Return true if the roach is not ready for another command

	bool isBusy();

	//------------------------------------------------------------
	// Methods which will be used to read and write register values.
	// These methods will send a the appropriate command to the servo,
	// and time out waiting for a response.
	//------------------------------------------------------------
	
	/**
	 * write message to serial port
	 */
	int writeString(std::string message);

      private:

	/**
	 * The file descriptor associated with the SERVO.
	 */
	int fd_;
	
	/**
	 * Original serial terminal settings
	 */
	struct termios termioSave_;
	
	/**
	 * The data registers we will store
	 */
	RegMapBlock* Utc_;        	    
	RegMapBlock* LL_;        	    
	RegMapBlock* RR_;        	    
	RegMapBlock* Q_;        	    
	RegMapBlock* U_;        	    
	RegMapBlock* TL1_;        	    
	RegMapBlock* TL2_;        	    
	
	/*
	RegMapBlock* azPositions_;          
	RegMapBlock* azErrors_;        	    
	RegMapBlock* elPositions_;          
	RegMapBlock* elErrors_;        	    
	RegMapBlock* slowAzPos_; 	    
	RegMapBlock* slowElPos_; 	    
	RegMapBlock* servoStatus_; 	    
	RegMapBlock* servoThermalCutouts_;  
	RegMapBlock* servoContactors_;	    
	RegMapBlock* servoCircuitBreakers_; 
	RegMapBlock* servoBrakes_; 	    
	RegMapBlock* driveLids_;	    
	RegMapBlock* azWrap_;               
	*/

	/**
	 * The set of file descriptors to be watched for readability.
	 */
	gcp::util::FdSet fdSet_;
	
	/**
	 * A not-so-private message container used for sending and
	 * receiving data from the SERVO.  need it to be public to
	 * check the presence of 1PPS
	 */
      public:
	/**
	 * True when we have an ethernet connection to the SERVO.
	 */
	bool connected_;
	
	bool sim_;




      private:
	/**
	 * True when we are expecting a response to a command.
	 */
	bool responsePending_;

	/**
	 * Service our message queue.
	 */
	void serviceMsgQ();
	
	/**
	 * Private method to zero the set of descriptors to be watched
	 * for readability.
	 */
	void zeroReadFds();
	
	/**
	 * Private method to register a file descriptor to be
	 * watched for input.
	 */
	void registerReadFd(int fd);
	
	/**
	 * Block, waiting for input from the servo or a message on our
	 * message queue, or for a timeout to occur, if relevant.
	 */
	int waitForNextMessage();

	/**
	 * Read bytes from the serial port and save the response
	 */
	int readTCPPort(RoachBackendMsg& command);


	/**
	 * Parse the vector response into a series of floats
	 **/
	int parseBackendResponse(RoachBackendMsg& command);

	/**
	 * Block, waiting for input from the servo.
	 */
	void waitForResponse();

	/**
	 * Record status response to the register map.
	 */
	void recordStatusResponse(RoachBackendMsg& command);

	/**
	 *  wait a bit
	 */
	void wait(long nsec=100000000);

	
      }; // End class RoachBackend
      
    }; // End namespace control
  }; // End namespace antenna
}; // End namespace gcp




#endif // End

