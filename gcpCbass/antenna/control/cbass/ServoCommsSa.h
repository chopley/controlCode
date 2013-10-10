#ifndef SERVOCOMMSSA_H
#define SERVOCOMMSSA_H

/**
 * @file ServoCommsSa.h
 * 
 * Tagged: Thu Nov 13 16:53:44 UTC 2003
 * 
 * @author Erik Leitch 
 *    modified for CBASS by Stephen Muchovej
 */
// Required C header files from the array control code

#include "gcp/control/code/unix/libunix_src/common/regmap.h" // RegMapBlock

#include "gcp/antenna/control/specific/AxisPositions.h"
#include "gcp/antenna/control/specific/Board.h"
#include "gcp/antenna/control/specific/Model.h"
#include "gcp/antenna/control/specific/ServoCommandSa.h"
#include "gcp/antenna/control/specific/PmacTarget.h"

#include "gcp/util/common/Angle.h"
#include "gcp/util/common/FdSet.h"

#include <termios.h>

#include <string>

/**
 * I have measured SERVO latency to be about 50 ms, or 50000 usec.
 * Quite slow, unfortunately.  We'll set the timeout to something
 * significantly larger than this.
 */
#define SERVO_TIMEOUT_USEC 150000 

namespace gcp {
  namespace antenna {
    namespace control {

      class SpecificShare;
      
      class ServoCommsSa : public Board {
	
      public:
	
	/**
	 * Constructor with pointer to shared resources.
	 */
	ServoCommsSa(SpecificShare* share, std::string name, bool sim);
	
	/**
	 * Constructor.  
	 */
	ServoCommsSa();
	
	/**
	 * Destructor.
	 */
	~ServoCommsSa();

	/**
	 * Command set -- commands called by higher functions
	 */
	void setAzEl(gcp::util::Angle& az, gcp::util::Angle& el);
	void setAzEl(gcp::util::Angle& az, gcp::util::Angle& el, gcp::util::Angle& az1, gcp::util::Angle& el1, gcp::util::Angle& az2, gcp::util::Angle& el2, gcp::util::TimeVal& mjd);
	void haltAntenna();
	void hardStopAntenna();
	void initializeAntenna();
	void queryStatus();
	void queryStatus(gcp::util::TimeVal& currTime);
	void queryAntPositions();
	void queryAntPositions(gcp::util::TimeVal& currTime);

	bool readPosition(AxisPositions* axes, Model* model);
	void commandNewPosition(PmacTarget* pmac);
	void commandNewPosition(PmacTarget* pmac, PmacTarget* pmac1, PmacTarget* pmac2, gcp::util::TimeVal& mjd);

	/**
	 *  Commands to probe parts of the status
	 */
	bool isCircuitBreakerTripped();
	bool isThermalTripped();
	bool isLidOpen();
	bool isBrakeOn();
	bool isSim();
	bool isInitialized();

	/* 
	 * Check for first part of initialization
	 */
	bool isPartOneComplete();

	/**
	 * last few steps of the initialization
	 */
	void finishInitialization();
	
	/**
	 * Fill the utc register with the time stamps appropriate for the data
	 * just read back from the servo box
	 */
	void fillUtc(gcp::util::TimeVal& currTime);


	/**
	 * Connect to the servo.  
	 *
	 * Returns true on success.
	 */
	bool connect();
	
	/**
	 * Disconnect from the servo.  
	 */
	void disconnect();

	/**
	 * Return true if the servo is connected.
	 */
	bool servoIsConnected();
	
	/**
	 * Send a single command to the SERVO.
	 */
	void sendCommand(ServoCommandSa& command);
	
	/**
	 * Read a response from the SERVO.  Does not check that a
	 * response is available, and may block waiting for one,
	 * depending on how the socket was configured.
	 */
	int readResponse(ServoCommandSa& command);

	// Return true if the servo is not ready for another command

	bool isBusy();

	unsigned readPositionFault();

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

	bool sim_;

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
	RegMapBlock* utc_;        	    
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

	/**
	 * The set of file descriptors to be watched for readability.
	 */
	gcp::util::FdSet fdSet_;
	
	/**
	 * True when we have an ethernet connection to the SERVO.
	 */
	bool connected_;
	
	/**
	 * A not-so-private message container used for sending and
	 * receiving data from the SERVO.  need it to be public to
	 * check the presence of 1PPS
	 */
      public:
	ServoCommandSa command_;

	/**
	 *  Issue a command, get a response, and check that it is valid.
	 */
	ServoCommandSa issueCommand(ServoCommandSa::Request req, std::vector<float>& values);
	ServoCommandSa issueCommand(ServoCommandSa::Request req);
	
      private:
	/**
	 * True when we are expecting a response to a command.
	 */
	bool responsePending_;

	/**
	 * 0 if mechanical check of initialization not good.
	 */
	bool initializationPartOne_;

	/**
	 * 0 if initialization not complete.  1 if complete.
	 */
	bool initializationComplete_;

	/**
	 * 0 if alarm not on, 1 if it's on
	 */
	bool alarmStatus_;

	/**
	 * 0 if antenna servo is engaged, 1 if antenn servo is disengaged
	 */
	bool antennaHalted_;
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
	int readTCPPort(ServoCommandSa& command);

	/**
	 * Block, waiting for input from the servo.
	 */
	void waitForResponse();

	/**
	 * Record status response to the register map.
	 */
	void recordStatusResponse(ServoCommandSa& command);

	/**
	 *  wait a bit
	 */
	void wait(long nsec=100000000);

	
      }; // End class ServoCommsSa
      
    }; // End namespace control
  }; // End namespace antenna
}; // End namespace gcp




#endif // End

