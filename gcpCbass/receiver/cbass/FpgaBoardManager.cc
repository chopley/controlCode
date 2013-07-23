#include "gcp/util/common/DataFrameNormal.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/receiver/specific/FpgaBoardManager.h"

#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"
#include "gcp/control/code/unix/libunix_src/common/scanner.h"

#include <iomanip>

using namespace gcp::receiver;
using namespace gcp::util;
using namespace std;

FpgaBoardManager::
FpgaBoardManager(bool archivedOnly) :
  XMLBoardDataFrameManager("array", "fpga", archivedOnly) 
{
  oscDemodMap_ = new OscDemodLookupMap(this);
  
  oscDemodMap_->add(oscDemodMap_->timeStampRegisters_, "PollTimestamp", "TimeOfWrite", "string");

  oscDemodMap_->digpotRegisters_->add("Digpot_RDAC_0_1");
  oscDemodMap_->digpotRegisters_->add( "Digpot_RDAC_2_3");
  
  oscDemodMap_->add(oscDemodMap_->globalRegisters_,  "BoardModbusAddress", "BoardModbusAddress", "int");
  oscDemodMap_->add(oscDemodMap_->globalRegisters_,  "BoardSerialNumber", "BoardSerialNumber", "int");
  oscDemodMap_->add(oscDemodMap_->globalRegisters_,  "Revision", "Revision", "int");     
    
  oscDemodMap_->globalRegisters_->add( "DDS_ModSel_B8_B1_31_16", 0x838);
  oscDemodMap_->globalRegisters_->add( "DDS_ModSel_A8_A1_15_0", 0x83a);
  oscDemodMap_->globalRegisters_->add( "Tayloe_CorrDub_En_B8_A1_15_0", 0x83c);
  oscDemodMap_->globalRegisters_->add( "DDS_SKEY_En_B8_A1_15_0", 0x83e);
  oscDemodMap_->globalRegisters_->add( "NullingOutParity_B8_A1_15_0", 0x880);
  oscDemodMap_->globalRegisters_->add( "ADC_Mode", 0x1000);
  oscDemodMap_->globalRegisters_->add( "ADC_AC_Chan_Sel_B8_A1_15_0", 0x1002);
  oscDemodMap_->globalRegisters_->add( "ADC_DC_Chan_Sel_B8_A1_15_0", 0x1004);
  oscDemodMap_->globalRegisters_->add( "ADC_AC_FIFO_ACQ_Limit", 0x1008);
  oscDemodMap_->globalRegisters_->add( "ADC_DC_FIFO_ACQ_Limit", 0x100a);
  oscDemodMap_->globalRegisters_->add( "ADC_Conv_Clk_Divider", 0x100c);
  oscDemodMap_->globalRegisters_->add( "Temp_and_ACD_Status_Reg", 0x1010);
  oscDemodMap_->globalRegisters_->add( "Demod_Gain_A8_A1_15_0", 0x1800);
  oscDemodMap_->globalRegisters_->add( "Demod_Gain_B8_B1_15_0", 0x1802);
  oscDemodMap_->globalRegisters_->add( "Carrier_Attn_A8_A1_15_0", 0x1804);
  oscDemodMap_->globalRegisters_->add( "Carrier_Attn_B8_B1_15_0", 0x1806);
  oscDemodMap_->globalRegisters_->add( "Nuller_Attn_A8_A1_15_0", 0x1808);
  oscDemodMap_->globalRegisters_->add( "Nuller_Attn_B8_B1_15_0", 0x180a);
  oscDemodMap_->globalRegisters_->add( "Demod_AC_Coup_B8_A1_15_0", 0x180c);
  oscDemodMap_->globalRegisters_->add( "SQUID_Power_B_A_3_0", 0x2000);
  oscDemodMap_->globalRegisters_->add( "SQUID_Clk_divide", 0x2002);
  oscDemodMap_->globalRegisters_->add( "Communication_Mode", 0x2800);
  oscDemodMap_->globalRegisters_->add( "Firmware_Format_Rev", 0x2804);
  oscDemodMap_->globalRegisters_->add( "Project_ID", 0x2806);
  oscDemodMap_->globalRegisters_->add( "Stream_Serial_ID_15_0", 0x2808);
  oscDemodMap_->globalRegisters_->add( "Stream_Serial_ID_31_16", 0x280a);
  oscDemodMap_->globalRegisters_->add( "Test_MUX_Select", 0xf000);
  oscDemodMap_->globalRegisters_->add( "Board_Rev_FPGA_Code_Rev", 0xf800);
  oscDemodMap_->globalRegisters_->add( "Board_Serial_No_9_0", 0xf802);

  oscDemodMap_->ddsRegisters_->add( "DDS_Phase_1_13_0");
  oscDemodMap_->ddsRegisters_->add( "DDS_Phase_2_13_0N");
  oscDemodMap_->ddsRegisters_->add( "DDS_Frequency_1_47_32");
  oscDemodMap_->ddsRegisters_->add( "DDS_Frequency_1_31_16");
  oscDemodMap_->ddsRegisters_->add( "DDS_Frequency_1_15_0");
  oscDemodMap_->ddsRegisters_->add( "DDS_Frequency_2_47_32N");
  oscDemodMap_->ddsRegisters_->add( "DDS_Frequency_2_31_16N");
  oscDemodMap_->ddsRegisters_->add( "DDS_Frequency_2_15_0N");
  oscDemodMap_->ddsRegisters_->add( "DDS_delta_Freq_47_32N");
  oscDemodMap_->ddsRegisters_->add( "DDS_delta_Freq_31_16N");
  oscDemodMap_->ddsRegisters_->add( "DDS_delta_Freq_15_0N");
  oscDemodMap_->ddsRegisters_->add( "DDS_Update_Clk_31_16");
  oscDemodMap_->ddsRegisters_->add( "DDS_Update_Clk_15_0");
  oscDemodMap_->ddsRegisters_->add( "DDS_Ramp_Rate_Clk_19_8N");
  oscDemodMap_->ddsRegisters_->add( "DDS_Ramp_7_0N");
  oscDemodMap_->ddsRegisters_->add( "DDSctrl1E_DDSctrl1F");
  oscDemodMap_->ddsRegisters_->add( "DDSctrl20_Amp_I_11_8");
  oscDemodMap_->ddsRegisters_->add( "DDS_Amp_I_11_8_Amp_Q_11_8");
  oscDemodMap_->ddsRegisters_->add( "DDS_Amp_Q_7_0_Shape_R_7_0N");
  oscDemodMap_->ddsRegisters_->add( "DDS_QDAC_11_0");
}

void FpgaBoardManager::OscDemodLookupMap::set(
  unsigned int addr, int value, unsigned int boardNum)
{
  static const unsigned int DIGPOT_BASE = 0x840;
  static const unsigned int GLOBAL_BASE = 0x838;
  static const unsigned int DDS_ADDRESS_INCREMENT = 0x80;
  if((addr >= DIGPOT_BASE) && 
     (addr < (DIGPOT_BASE + (4 * NUM_BOLOMETERS_PER_BOARD)))) {

    unsigned int offset = addr - DIGPOT_BASE;
    unsigned int channel = offset / 4;
    unsigned int lowHigh = (offset / 2) & 1; // Register number in list of digpot registers

    digpotRegisters_->set(lowHigh, value, boardNum, channel);
  } else if(addr >= GLOBAL_BASE) {
    globalRegisters_->set(addr, value, boardNum);
  } else if(addr < (DDS_ADDRESS_INCREMENT * NUM_BOLOMETERS_PER_BOARD)) {
    unsigned int channel = addr / DDS_ADDRESS_INCREMENT;
    unsigned int index = (addr % DDS_ADDRESS_INCREMENT) / 2;
    ddsRegisters_->set(index, value, boardNum, channel);
  }
}

void FpgaBoardManager::OscDemodLookupMap::
set(unsigned int addr, double value, unsigned int boardNum) {}

void FpgaBoardManager::OscDemodLookupMap::
set(std::string name, int value, unsigned int boardNum)
{
  globalRegisters_->set(name, value, boardNum);
}

void FpgaBoardManager::OscDemodLookupMap::
set(std::string name, std::string value, unsigned int boardNum)
{
  if(name == "TimeOfWrite")
  {
    TimeVal timeStamp = TimeVal(value, "%a %b %d %T %Y");
    RegDate date = RegDate(timeStamp);
    timeStampRegisters_->set("TimeOfWrite", *date.data(), boardNum);
  }
}

void FpgaBoardManager::OscDemodLookupMap::write()
{
  timeStampRegisters_->write();
  digpotRegisters_->write();
  globalRegisters_->write();
  ddsRegisters_->write();
}
   
void FpgaBoardManager::
setTo(MuxReadout::MuxXMLFile *xml,
      std::vector<ReceiverConfigConsumer::Board>& boards)
{
  RegDate date;
  date.setToCurrentTime();
  setMjd(date);
  setReceived(true);
  incrementRecord();

  unsigned int boardNum = 0;
   
  for(unsigned iBoard=0; iBoard < boards.size(); iBoard++) {

    std::ostringstream board_id;
    board_id << "RegisterMap_" << std::hex << boards[iBoard].id_ << ":";
    vector<std::string> oscDemodPath;

    oscDemodPath.push_back("CmdResp");
    oscDemodPath.push_back(board_id.str());
    oscDemodPath.push_back("OscDemodBoardRegisters");

    getRegisters(xml, oscDemodPath,
		 "PersistantRegisterAddresses",
		 "PersistantRegisterValues",
		 *oscDemodMap_, iBoard);
  }
}

FpgaBoardManager::OscDemodLookupMap::
OscDemodLookupMap(BoardDataFrameManager* frame) : frame_(frame) {
  timeStampRegisters_ = new RegisterList<RegDate::Data>(frame_, NUM_BOARDS);

  ddsRegisters_       = 
    new RegisterList<unsigned short>(frame_, NUM_BOARDS, 
				     NUM_BOLOMETERS_PER_BOARD);

  globalRegisters_    = new RegisterList<unsigned short>(frame_, NUM_BOARDS);

  digpotRegisters_    = 
    new RegisterList<unsigned short>(frame_, NUM_BOARDS, 
				     NUM_BOLOMETERS_PER_BOARD);
}
