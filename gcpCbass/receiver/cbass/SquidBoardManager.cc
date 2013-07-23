#include "gcp/util/common/DataFrameNormal.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/receiver/specific/SquidBoardManager.h"

#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"
#include "gcp/control/code/unix/libunix_src/common/scanner.h"

#include <iomanip>

using namespace gcp::receiver;
using namespace gcp::util;
using namespace std;

SquidBoardManager::
SquidBoardManager(bool archivedOnly) :
  XMLBoardDataFrameManager("array", "squidController", archivedOnly)
{
  squidMap_    = new SquidLookupMap(this);
  
  //=======================================================================
  // Store the map of squid registers
  //=======================================================================

  squidMap_->add(squidMap_->timeStampRegisters_,
		 "PollTimestamp", "TimeOfWrite", "string");
    
  squidMap_->add(squidMap_->globalRegisters_,  
		 "BoardModbusAddress", "BoardModbusAddress", "int");
  squidMap_->add(squidMap_->globalRegisters_,  
		 "BoardSerialNumber", "BoardSerialNumber", "int");
  squidMap_->add(squidMap_->globalRegisters_,  
		 "Id", "Id", "int");
  squidMap_->add(squidMap_->globalRegisters_,  
		 "Port", "Port_1forA0forB", "bool");
  squidMap_->add(squidMap_->globalRegisters_,  
		 "Revision", "Revision", "int");     
    
  squidMap_->globalRegisters_->add(  "Board_Id", 0);
  squidMap_->globalRegisters_->add(  "DAC_Board_Offset", 60);
  
  squidMap_->channelRegisters_->add( "DAC_Heater");
  squidMap_->channelRegisters_->add( "DAC_Offset");
  squidMap_->channelRegisters_->add( "DAC_Flux_Bias");
  squidMap_->channelRegisters_->add( "DAC_Squid_Bias");
  squidMap_->channelRegisters_->add( "Switch");
  squidMap_->channelRegisters_->add( "Switch_Enable");
}

void SquidBoardManager::SquidLookupMap::set(
  unsigned int addr, int value, unsigned int boardNum)
{
  if((addr == 0) || (addr == 60)) {
    globalRegisters_->set(addr, value, boardNum);
  } else {
    for(unsigned int startIndex = 1; startIndex <= 51; startIndex += 10) {
      if((addr >= startIndex) && (addr <= (startIndex + 8))) {
        channelRegisters_->set((startIndex - 1) / 10, value, 
			       boardNum, addr - startIndex);
      }
    }
  }
}

void SquidBoardManager::SquidLookupMap::
set(unsigned int addr, double value, unsigned int boardNum) {}

void SquidBoardManager::SquidLookupMap::
set(std::string name, int value, unsigned int boardNum)
{
  globalRegisters_->set(name, value, boardNum);
}

void SquidBoardManager::SquidLookupMap::
set(std::string name, std::string value, unsigned int boardNum)
{
  if(name == "TimeOfWrite")
  {
    TimeVal timeStamp = TimeVal(value, "%a %b %d %T %Y");
    RegDate date = RegDate(timeStamp);
    timeStampRegisters_->set("TimeOfWrite", *date.data(), boardNum);
  }
}

void SquidBoardManager::SquidLookupMap::write()
{
  timeStampRegisters_->write();
  channelRegisters_->write();
  globalRegisters_->write();
}

void SquidBoardManager::
setTo(MuxReadout::MuxXMLFile *xml,
      std::vector<ReceiverConfigConsumer::Board>& boards)
{
  RegDate date;
  date.setToCurrentTime();
  setMjd(date);
  setReceived(true);
  incrementRecord();

  unsigned int boardNum = 0;
   
  unsigned iSquid=0;
  for(unsigned iBoard=0; iBoard < boards.size(); iBoard++) {

    if(boards[iBoard].squid_ != ReceiverConfigConsumer::SQUID_NONE) {

      ReceiverConfigConsumer::Board& brd = boards[iBoard];
      std::ostringstream squid_id;
      squid_id << "RegisterMap_" << std::hex << brd.id_ << ":"
	       << (brd.squid_==ReceiverConfigConsumer::SQUID_A ? "sa" : "sb");

      vector<string> squidPath;
      
      squidPath.push_back("CmdResp");
      squidPath.push_back(squid_id.str());
      squidPath.push_back("SQControllerRegisters");
      
      getRegisters(xml, squidPath,
		   "PersistantRegisterAddresses",
		   "PersistantRegisterValues",
		   *squidMap_, iSquid);

      ++iSquid;
    }
  }
}

SquidBoardManager::SquidLookupMap::
SquidLookupMap(BoardDataFrameManager* frame) : frame_(frame) {

  timeStampRegisters_ = 
    new RegisterList<gcp::util::RegDate::Data>(frame_, NUM_BOARDS);

  globalRegisters_    = 
    new RegisterList<unsigned short>(frame_, NUM_BOARDS);

  channelRegisters_   = 
    new RegisterList<unsigned short>(frame_, NUM_BOARDS, 
				     NUM_BOLOMETERS_PER_BOARD / 2);
}
