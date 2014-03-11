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
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <termios.h>
#include <string>
#include "gcp/util/common/String.h"

#include <vector>

#include "gcp/util/common/FdSet.h"
#include "gcp/util/common/Directives.h"
#include "gcp/antenna/control/specific/RoachBackendMsg.h"
#include "gcp/control/code/unix/libunix_src/common/regmap.h" // RegMapBlock
#include "gcp/antenna/control/specific/Board.h"
#include <termios.h>
#include <string>


/**                                                                                                                                  
 *  LATENCY FOR ROACH BACKEND                                                                                                        
 */
#define ROACH_TIMEOUT_USEC 10000
#define RING_BUFFER_LENGTH 300  // size of our buffer

namespace gcp {
  namespace antenna {
    namespace control {

      class SpecificShare;
      
      class RoachBackend : public Board {
	
      public:
	
	/**
	 * Constructor with pointer to shared resources.
	 */
	RoachBackend(bool sim=false, char* controllerName="pumba");
	RoachBackend(SpecificShare* share, std::string name, bool sim=false, char* controllerName="pumba", int roachNum=1);
	void Assign3DRingMemory();
	void Assign2DRingMemory();
	
	/**
	 * Destructor 
	 */ 
	virtual ~RoachBackend();
	
	/**
	 * function to connect
	 */ 
	bool connect();
	
	/**
	 *  missed comm counter
	 */  
	int missedCommCounter_;

	/**
	 *  Controller name
	 */  
	char* controllerName_;

	/**
	 * Roach Index (for writing data out to proper location
	 **/
	int roachIndex_;

	/**
	 * disconnect
	 */
	void disconnect();
	
	/**
	 * if we are connected
	 */ 
	bool connected_;
	bool roachIsConnected();
	
	/**
	 *  all our data for this transfer will be in RoachBackendMsg object
	 */
	RoachBackendMsg command_;
	
	/**
         * Block, waiting for input from the roach
         */
        int waitForResponse();

        /**
         *  wait a bit 
         */
        void wait(int nsec=100000000);

	/**
	 *  Generalized pack&issue command 
	 */
	int issueCommand(RoachBackendMsg& command);

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
	
	/**
	 *   Parse a full data packet from the backend
	 **/
	int parseBackendDataPacket(RoachBackendMsg& command);

	/**
	 *  Parse response to specific command of backend
	 **/
	int parseBackendResponse(RoachBackendMsg& command);
	
	/**
	 *  Check value of response
	 **/ 
	void checkOneOutput(RoachBackendMsg& command);
	
	/**
	 *   wrapper to get data from the backend
	 **/ 
	void getData();
	
	/**
	 *   wrapper to issue command to backend
	 **/ 
	void sendMessage(RoachBackendMsg message);
	
	/**
	 * write the data to disk
	 **/
	void writeData(gcp::util::TimeVal& currTime);
	void writeData3D(gcp::util::TimeVal& currTime);

	//      private:
	void printBits(unsigned char feature);

	/**
	 * Private function to read/write to roach
	 */ 
	int writeString(std::string message);
        int readTCPPort(RoachBackendMsg& command);
	
	/**                                                                             
         * The file descriptor associated with the roach
         */
        int fd_;


	/**
	 * The set of file descriptors to be watched for readability.
	 */
	gcp::util::FdSet fdSet_;
	/**
	 *  Are we simulating the backend?
	 **/ 
	bool sim_;

	/**           
         * Original serial terminal settings                                            
         */
	struct termios termioSave_;


	/**
	 * Ring buffer with all our data which we will fill up and write out
	 **/
	std::vector<int> version_;   // roach version number                                                   
	std::vector<int> packetSize_;// numbers of bytes in the transfer                                       
        std::vector<int> numFrames_; // number of frames being transfer                                        
	std::vector<int> intCount_;  // number of accumulation on fpga                                         
	std::vector<int> bufferBacklog_;  // buffer backlog on the Roach                                         
        std::vector<int> tstart_;    // start time of integration                                              
        std::vector<int> switchstatus_;    // switching status Noise Diode etc                                              
        std::vector<int> tstop_;     // stop time of integration                                               
        std::vector<int> intLength_; // integration length in clock cycles                                     
        std::vector<int> mode_;      // backend mode (polarization or power);                                  
	std::vector<int> res2_;      // reserved 4 bytes                                                       
	std::vector<int> seconds_;      // seconds from the roach                                                       
	std::vector<int> useconds_;      // useconds from the roach                                                       
	std::vector<float> Coeffs_;

#if(0) // 3D stuff
	std::vector<std::vector< std::vector<float> > > LL_;
	std::vector<std::vector<std::vector<float> > > RR_;
	std::vector<std::vector<std::vector<float> > > Q_;
	std::vector<std::vector<std::vector<float> > > U_;
	std::vector<std::vector<std::vector<float> > > TL1_;
	std::vector<std::vector<std::vector<float> > > TL2_;
#endif
	std::vector<std::vector<float> > LL_;
	std::vector<std::vector<float> > RR_;
	std::vector<std::vector<float> > Q_;
	std::vector<std::vector<float> > U_;
	std::vector<std::vector<float> > TL1_;
	std::vector<std::vector<float> > TL2_;

	int currentIndex_;
        int prevSecStart_;
        int prevSecEnd_;
        int thisSecStart_;
        float prevTime_;

	/**                                                                                                            
         *  data registers to store                                                                                    
         */
	RegMapBlock* roachUtc_;       
        RegMapBlock* roachVersion_;   
        RegMapBlock* roachCount_;     
        RegMapBlock* roachIntLength_; 
        RegMapBlock* roachBufferBacklog_; 
        RegMapBlock* roachMode_;      
	RegMapBlock* roachSwitchStatus_;        
	RegMapBlock* roachLL_;        
        RegMapBlock* roachRR_;        
        RegMapBlock* roachQ_;         
        RegMapBlock* roachU_;         
	RegMapBlock* roachTL1_;       
	RegMapBlock* roachTL2_;      
	RegMapBlock* roachChan_;   
	RegMapBlock* roachLLfreq_; 
	RegMapBlock* roachRRfreq_; 
	RegMapBlock* roachQfreq_;  
	RegMapBlock* roachUfreq_;  
	RegMapBlock* roachTL1freq_;
	RegMapBlock* roachTL2freq_;
	RegMapBlock* roachLLtime_; 
	RegMapBlock* roachRRtime_; 
	RegMapBlock* roachQtime_;  
	RegMapBlock* roachUtime_;  
 	RegMapBlock* roachTL1time_;
	RegMapBlock* roachTL2time_;
	RegMapBlock* roachNTPSeconds_;
	RegMapBlock* roachNTPuSeconds_;       
	RegMapBlock* roachFPGAClockStamp_;       
	RegMapBlock* roachCoffs_;       


      }; // End class RoachBackend
    }; // End namespace control
  }; // End namespace antenna
}; // End namespace gcp
#endif

