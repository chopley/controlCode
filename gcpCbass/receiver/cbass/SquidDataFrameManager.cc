#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/AxisRange.h"
#include "gcp/util/common/DataFrameNormal.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Directives.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/FrameFlags.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/receiver/specific/SquidDataFrameManager.h"

#include "gcp/control/code/unix/libunix_src/common/scanner.h"

#include <iomanip>

using namespace gcp::util;
using namespace gcp::receiver;
using namespace std;

/**.......................................................................
 * Constructor with initialization from a DataFrame object.
 */
SquidDataFrameManager::SquidDataFrameManager() :
  BoardDataFrameManager("receiver", "squids", false)
{
  voltsPtr_ = getReg("adc");
  idPtr_    = getReg("id");
}

/**.......................................................................
 * Destructor
 */
SquidDataFrameManager::~SquidDataFrameManager()
{}

void SquidDataFrameManager::bufferVolts(float val, unsigned iSquid)
{
  volts_[iSquid] = val;

  if(iSquid == NUM_SQUIDS-1) {
    writeBoardReg(voltsPtr_, volts_);
  }
}

void SquidDataFrameManager::bufferId(std::string& id, unsigned iSquid)
{
  strncpy((char*)(ids_ + iSquid * DIO_ID_LEN), (char*)id.c_str(), DIO_ID_LEN);

  if(iSquid == NUM_SQUIDS-1)
    writeBoardReg(idPtr_,    ids_);
}


void SquidDataFrameManager::setMjd(gcp::util::RegDate& date)
{
  writeBoardReg("utc", date.data());
}

void SquidDataFrameManager::setSubSampling(unsigned sampling)
{
  writeBoardReg("subSampling", &sampling);
}

void SquidDataFrameManager::setFilterNtap(unsigned ntaps)
{
  writeBoardReg("ntap", &ntaps);
}

