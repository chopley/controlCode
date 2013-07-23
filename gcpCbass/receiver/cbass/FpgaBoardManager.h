#ifndef GCP_RECEIVER_FPGABOARDMANAGER_H
#define GCP_RECEIVER_FPGABOARDMANAGER_H

/**
 * @file FpgaBoardManager.h
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
      
      class FpgaBoardManager : 
	public gcp::receiver::XMLBoardDataFrameManager {

      public:
	
        FpgaBoardManager(bool archivedOnly=false);

	void setTo(MuxReadout::MuxXMLFile *xml, 
		   std::vector<ReceiverConfigConsumer::Board>& boards);
        
        class OscDemodLookupMap : public XMLRegisterMap {
        
        public:
        
	  OscDemodLookupMap() : frame_(NULL) {};
	  OscDemodLookupMap(BoardDataFrameManager* frame);

          void set(unsigned int addr, int value, unsigned int boardNum);
          void set(unsigned int addr, double value, unsigned int boardNum);
          void set(std::string name, int value, unsigned int boardNum);
          void set(std::string name, std::string value, unsigned int boardNum);
  
          void write();
        
          RegisterList<gcp::util::RegDate::Data>*  timeStampRegisters_;
          RegisterList<unsigned short>* ddsRegisters_;
          RegisterList<unsigned short>* globalRegisters_;
          RegisterList<unsigned short>* digpotRegisters_;
	  
	  private:
	  
	  BoardDataFrameManager* frame_;
        };
        
      private:
      
        OscDemodLookupMap* oscDemodMap_;
      };
      
  } // End namespace receiver
} // End namespace gcp

#endif // End #ifndef GCP_RECEIVER_FPGABOARDMANAGER_H
