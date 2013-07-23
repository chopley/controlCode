#define __FILEPATH__ "util/common/AntennaDataFrame.cc"

#include "gcp/util/common/AntennaDataFrame.h"
#include "gcp/util/common/Debug.h"

using namespace gcp::util;

/**.......................................................................
 * Constructor
 */
AntennaDataFrame::AntennaDataFrame() 
{
  DBPRINT(false, Debug::DEBUG7, "Inside AntennaDataFrame constructor: "
	  << antNum_);
};

/**.......................................................................
 * Constructor with antenna enumerator
 */
AntennaDataFrame::AntennaDataFrame(const gcp::util::AntNum& antNum) :
  antNum_(antNum) {};

/**.......................................................................
 * Destructor
 */
AntennaDataFrame::~AntennaDataFrame() {};

/**.......................................................................
 * Set the antenna number associated with this data frame.
 */
void AntennaDataFrame::setAnt(unsigned int id)
{
  antNum_.setId(id);
}

/**.......................................................................
 * Set the antenna number associated with this data frame.
 */
void AntennaDataFrame::setAnt(AntNum::Id antennaId)
{
  antNum_.setId(antennaId);
}

/**.......................................................................
 * Set the antenna number associated with this data frame.
 */
void AntennaDataFrame::setAnt(const AntNum& antNum)
{
  antNum_.setId(antNum);
}

/**.......................................................................
 * Get the antenna number associated with this data frame.
 */
unsigned int AntennaDataFrame::getAntIntId()
{
  return antNum_.getIntId();
}

/**.......................................................................
 * Get the antenna number associated with this data frame.
 */
AntNum AntennaDataFrame::getAnt()
{
  return antNum_;
}

/**.......................................................................
 * Assignment operator.
 */
void AntennaDataFrame::operator=(AntennaDataFrame& frame)
{
  // Just assign our members

  antNum_ = frame.antNum_;
}
