#ifndef SERVOCOMMS_H
#define SERVOCOMMS_H

/**
 * @file ServoComms.h
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
#include "gcp/antenna/control/specific/ServoCommand.h"
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
      
      class ServoComms : public Board {
	
      public:
	
	/**
	 * Constructor with pointer to shared resources.
	 */
	ServoComms(SpecificShare* share, std::string name, bool sim);
	
	/**
	 * Constructor.  
	 */
	ServoComms();
	
	/**
	 * Destructor.
	 */
	~ServoComms();

	/**
	 * Command set -- commands called by higher functions
	 */
	void setAzEl(gcp::util::Angle& az, gcp::util::Angle& el);
	void hardStopAntenna(); //  not nice on drives.  don't use.
	void haltAntenna();
	void calibrateEncoders();
	void initializeAntenna(int antNum=1);
	void queryStatus();
	void queryStatus(gcp::util::TimeVal& currTime);
	void queryAntPositions();
	void queryAntPositions(gcp::util::TimeVal& currTime);

	bool readPosition(AxisPositions* axes, Model* model);
	void commandNewPosition(PmacTarget* pmac);

	/**
	 *  set maximum velocity
	 */
	void setMaxAzVel(int antNum, float vMax);

	/**
	 * last few steps of the initialization
	 */
	void finishInitialization();
	

	/**
	 *  Count of how many consecutive messages communications have hung on
	 */
	int inactiveCount_;


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
	void sendCommand(ServoCommand& command);
	
	/**
	 * Read a response from the SERVO.  Does not check that a
	 * response is available, and may block waiting for one,
	 * depending on how the socket was configured.
	 */
	int readResponse(ServoCommand& command);

	// Return true if the servo is not ready for another command

	bool isBusy();

	// Return true if we have a 1PPS
	bool isPpsPresent();

	// Return true if the encoders are calibrated
	bool isCalibrated();

	// Returns the value of the tracking loop (1 -TON, 3 - Cal encoders, 5 - Time loop)
	int checkTrackLoop();

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
	RegMapBlock* errorCount_;
	RegMapBlock* slowAzPos_; 
	RegMapBlock* slowElPos_; 
	RegMapBlock* az1CommI_;  
	RegMapBlock* az2CommI_;  
	RegMapBlock* el1CommI_;  
	RegMapBlock* az1ActI_;   
	RegMapBlock* az2ActI_;   
	RegMapBlock* el1ActI_;   
	RegMapBlock* az1Enable_; 
	RegMapBlock* az2Enable_; 
	RegMapBlock* el1Enable_; 
	RegMapBlock* taskLoop_; 
	RegMapBlock* encStatus_;   
	RegMapBlock* lowSoftAz_;
	RegMapBlock* lowHardAz_;
	RegMapBlock* hiSoftAz_; 
	RegMapBlock* hiHardAz_; 
	RegMapBlock* lowSoftEl_;
	RegMapBlock* lowHardEl_;
	RegMapBlock* hiSoftEl_; 
	RegMapBlock* hiHardEl_; 
	RegMapBlock* azWrap_; 
	RegMapBlock* azBrake_; 
	RegMapBlock* elBrake_; 
	RegMapBlock* eStop_; 
	RegMapBlock* simulator_; 
	RegMapBlock* ppsPresent_; 

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
	ServoCommand command_;

      private:
	/**
	 *  Issue a command, get a response, and check that it is valid.
	 */
	ServoCommand issueCommand(ServoCommand::Request req, std::vector<float>& values);
	ServoCommand issueCommand(ServoCommand::Request req);
	
	/**
	 * True when we are expecting a response to a command.
	 */
	bool responsePending_;

	/**
	 * 0 if encoders not calibrated, 1 if calibrating, 2 when calibrated.
	 */
	int calibrationStatus_;

	/**
	 * 0 if initialization not complete.  1 if complete.
	 */
	bool initializationComplete_;

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
	int readSerialPort(ServoCommand& command);

	/**
	 * Block, waiting for input from the servo.
	 */
	void waitForResponse();

	/**
	 * Record status response to the register map.
	 */
	void recordStatusResponse(ServoCommand& command);

	/**
	 *  wait a bit
	 */
	void wait(long nsec=100000000);

	
      }; // End class ServoComms
      
    }; // End namespace control
  }; // End namespace antenna
}; // End namespace gcp




#endif // End #ifndef 


