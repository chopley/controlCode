#ifndef GCP_RECEIVER_RECEIVERCONFIGCONSUMER_H
#define GCP_RECEIVER_RECEIVERCONFIGCONSUMER_H

/**
 * @file ReceiverConfigConsumer.h
 * 
 * Tagged: Fri Feb  2 02:56:10 NZDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:24 $
 * 
 * @author Erik Leitch
 */

#include <iostream>

#include "gcp/receiver/specific/XMLConsumer.h"

#include "Utilities/HardwareManagerClient.h"

namespace gcp {

  namespace mediator {
    class Scanner;
  }

  namespace receiver {

    class FpgaBoardManager;
    class SquidBoardManager;

    class ReceiverConfigConsumer : public gcp::receiver::XMLConsumer {
    public:
      
      // Enumerate the types of commands we will send to the HWM

      enum Command {
	TYPE_NONE,      // No command
	TYPE_GETBOARDS, // GetBoards command
	TYPE_GETSQUIDS, // GetSquids command
	TYPE_GETSEQID,  // GetSeqId command
	TYPE_GETREGS    // GetAllRegisters command
      };

      // Enumerate the types of squid controller IDs that boards can have

      enum SquidId {
	SQUID_NONE = 0x0,
	SQUID_A    = 0x1,
	SQUID_B    = 0x2,
      };

      struct Board {
	int id_;
	SquidId squid_;

	Board() {
	  id_    = -1;
	  squid_ = SQUID_NONE;
	}

	Board(unsigned id, SquidId squid) {
	  id_    = id;
	  squid_ = squid;
	}

	friend ostream& operator<<(ostream& os, const Board& brd);
      };

      /**
       * Constructor.
       */
      ReceiverConfigConsumer(gcp::mediator::Scanner* scanner, 
			     std::string host="localhost",
			     unsigned short port=5207,
			     bool send_connect_request=true);
      
      // Destructor.

      virtual ~ReceiverConfigConsumer();

      // Get the next set of data from the server
      
      bool getData();

      // Request the list of boards

      bool requestBoards();

      // Request the list of boards

      bool ReceiverConfigConsumer::requestSquids();

      // Request the hardware configuration status (has anything
      // changed?)
      
      bool ReceiverConfigConsumer::requestHardwareStatus();

      // Request configuration data fgrom the HardwareManager

      bool ReceiverConfigConsumer::requestHardwareConfiguration();

      // Generic method to send a command to the HWM

      bool sendCommand(MuxReadout::HardwareManagerClient::Cmd* cmd, 
		       Command type);

      // Read configuration data from the HardwareManager

      bool ReceiverConfigConsumer::readResponse();

      // Return the last command we sent
      
      MuxReadout::HardwareManagerClient::Cmd* getLastSentCommand();

      bool readBoardInfo(MuxReadout::HardwareManagerClient::Cmd* cmd);

      bool readSquidInfo(MuxReadout::HardwareManagerClient::Cmd* cmd);

      bool readHardwareStatus(MuxReadout::HardwareManagerClient::Cmd* cmd);

      bool readHardwareConfiguration(MuxReadout::HardwareManagerClient::Cmd* cmd);
      // Report a connection error

      void reportError();

      // Report connection success

      void reportSuccess();

      // Main loop -- service our message queue

      void serviceMsgQ();

    private:

      MuxReadout::HardwareManagerClient::Cmd* getBoardsCmd_;
      MuxReadout::HardwareManagerClient::Cmd* getSquidsCmd_;
      MuxReadout::HardwareManagerClient::Cmd* getSeqIdCmd_;
      MuxReadout::HardwareManagerClient::Cmd* getRegsCmd_;

      int seqId_;

      std::vector<Board> boards_;
      bool haveBoardInfo_;
      bool getDataPending_;

      Command type_;

      FpgaBoardManager* fpgaBm_;
      SquidBoardManager* squidBm_;

    }; // End class ReceiverConfigConsumer

  } // End namespace receiver
} // End namespace gcp



#endif // End #ifndef GCP_RECEIVER_RECEIVERCONFIGCONSUMER_H
