#ifndef BACKEND_H
#define BACKEND_H

/**
 * @file Backend.h
 * 
 * Tagged: Thu Nov 19 16:53:44 UTC 2009
 * 
 * @author Stephen Muchovej
 */
// Required C header files from the array control code
#include "gcp/control/code/unix/libunix_src/common/regmap.h" // RegMapBlock
#include "gcp/antenna/control/specific/Board.h"
#include <termios.h>
#include <string>
#include "gcp/util/specific/CbassBackend.h"

#define MAX_BUFFER_LENGTH 300
namespace gcp {
  namespace antenna {
    namespace control {
      
      class SpecificShare;
      
      class Backend : public Board {

      public:
	
	/**
	 * Constructor with pointer to shared resources.
	 */
	Backend(SpecificShare* share, std::string name);
	
	/**
	 * Constructor.  
	 */
	Backend();
	
	/**
	 * Destructor.
	 */
	~Backend();

	/**
	 * Backend object
	 */ 
	gcp::util::CbassBackend cbassBackend_;

	/**
	 *  connect/disconnect
	 */ 
	void connect();
	void disconnect();

	/**
	 * send a command, check if it's valid.
	 */ 
	void issueCommand(gcp::util::CbassBackend::Command command);
	void issueCommand(gcp::util::CbassBackend::Command command, unsigned char address);
	void issueCommand(gcp::util::CbassBackend::Command command, unsigned char* period);
	void issueCommand(gcp::util::CbassBackend::Command command, unsigned char address, unsigned char* period);
	void issueCommand(gcp::util::CbassBackend::Command command, unsigned char address, unsigned char* period, unsigned char channel, unsigned char stage);

	/**
	 * Gets Data from the Backend
	 */
	void getData();
	void getBurstData();

	/**
	 * Write Data from the previous full second.
	 */
	void writeData(gcp::util::TimeVal& currTime);
	void writeData2011(gcp::util::TimeVal& currTime);

	/**
	 * Return true if the backend is connected.
	 */
	bool isConnected();

	/**
	 *  variables
	 */
	bool isValid_;
	bool connected_;
	int currentIndex_;
	int prevSecStart_;
	int prevSecEnd_;
	int thisSecStart_;
	float prevTime_;

	int missedCommCounter_;  // when this goes above 15, we're not connected

	std::vector<float> timeBuffer_;
	std::vector<float> versionBuffer_;
	std::vector<uint> avgSecBuffer_;
	std::vector<std::vector<float> > dataBuffer_;
	std::vector<std::vector<float> > regDataBuffer_;
	std::vector<std::vector<float> > diagnosticsBuffer_;
	std::vector<std::vector<float> > alphaBuffer_;
	std::vector<std::vector<float> > nonlinBuffer_;
	std::vector<unsigned short> flagBuffer_;

	/**
	 *  data registers to store
	 */
	RegMapBlock* rxUtc_;
	RegMapBlock* rxData_;
	RegMapBlock* rxFlags_;
	RegMapBlock* rxSwitchData_;
	RegMapBlock* rxDiagnostics_;
	RegMapBlock* rxVersion_;
	RegMapBlock* rxSecLength_;
	RegMapBlock* rxNonlin_;
	RegMapBlock* rxAlpha_;

      private:
	// check if return value is valid.
	bool checkValid(int retVal);
	
      }; // end Backend class
    };  // end namespace control
  };  // end namespace antenna
};   // end namespace gcp

#endif
