#ifndef GCP_RECEIVER_SQUIDBOARDMANAGER_H
#define GCP_RECEIVER_SQUIDBOARDMANAGER_H

/**
 * @file SquidBoardManager.h
 * 
 * Tagged: Wed Mar 31 09:59:54 PST 2004
 * 
 * @author Ken Aird
 */

// C header files from the array control code

#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"

#include "receiver/specific/ReceiverConfigConsumer.h"
#include "receiver/specific/XMLBoardDataFrameManager.h"

#include "Utilities/MuxXMLDatacard.h"

#include <map>
#include <vector>

namespace gcp {
  namespace receiver {
      
      class DataFrame;
      
      class SquidBoardManager : 
	public gcp::receiver::XMLBoardDataFrameManager {

      public:
	
        SquidBoardManager(bool archivedOnly=false);

	void setTo(MuxReadout::MuxXMLFile *xml, 
		   std::vector<ReceiverConfigConsumer::Board>& boards);
        
        class SquidLookupMap : public XMLRegisterMap {
	
	public:
	
	  SquidLookupMap() : frame_(NULL) {};
	  SquidLookupMap(BoardDataFrameManager* frame);
	  
          void set(unsigned int addr, int value, unsigned int boardNum);
          void set(unsigned int addr, double value, unsigned int boardNum);
          void set(std::string name, int value, unsigned int boardNum);
          void set(std::string name, std::string value, unsigned int boardNum);
          void write();
	  
	  RegisterList<gcp::util::RegDate::Data>* timeStampRegisters_;
          RegisterList<unsigned short>* globalRegisters_;
          RegisterList<unsigned short>* channelRegisters_;

	private:
	  
	  gcp::util::BoardDataFrameManager* frame_;
        };
        
      private:
      
        SquidLookupMap* squidMap_;
      };
      
  } // End namespace receiver
} // End namespace gcp

#endif // End #ifndef GCP_RECEIVER_SQUIDBOARDMANAGER_H
