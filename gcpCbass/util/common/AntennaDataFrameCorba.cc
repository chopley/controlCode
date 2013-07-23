#define __FILEPATH__ "util/common/AntennaDataFrameCorba.cc"

#include "gcp/util/common/AntennaDataFrameCorba.h"
#include "gcp/util/common/Debug.h"

using namespace gcp::util;

/** .......................................................................
 * Constructor
 */
AntennaDataFrameCorba::AntennaDataFrameCorba() 
{
  DBPRINT(false, Debug::DEBUG7, "Inside AntennaDataFrameCorba constructor");
};

/**.......................................................................
 * Constructor with buffer initialization.
 */
AntennaDataFrameCorba::AntennaDataFrameCorba(unsigned int nBuffer)
{
  resize(nBuffer);
};

/**.......................................................................
 * Destructor.
 */
AntennaDataFrameCorba::~AntennaDataFrameCorba() {};

#if DIR_USE_ANT_CORBA
/** .......................................................................
 * Constructor
 */
AntennaDataFrameCorba::
AntennaDataFrameCorba(const gcp::antenna::corba::DataFrame* frame) 
{
  resize(frame->nvals_);

  // Copy the data

  frame_.lvals_ = frame->lvals_;

  // And set the antenna id

  setAnt(frame->antennaId_);
};
#endif

/**.......................................................................
 * Resize the data buffer
 */
void AntennaDataFrameCorba::resize(unsigned int nBuffer)
{
  DBPRINT(false, Debug::DEBUG7, "Inside AntennaDataFrameCorba::resize(): "
	  << nBuffer);

#if DIR_USE_ANT_CORBA  
  frame_.lvals_.length(nBuffer);
  frame_.nvals_ = nBuffer;
#endif

  // And resize the shadow array

  lvals_.resize(nBuffer);
};

/**.......................................................................
 * Return the size of the internal buffer.
 */
unsigned int AntennaDataFrameCorba::size() 
{
#if DIR_USE_ANT_CORBA
  return frame_.nvals_;
#else
  return lvals_.size();
#endif
};

/**.......................................................................
 * Set the antenna id associated with this data frame
 */
void AntennaDataFrameCorba::setAnt(unsigned int antennaId)
{
  AntennaDataFrame::setAnt(antennaId);
#if DIR_USE_ANT_CORBA
  frame_.antennaId_ = antNum_.getIntId();
#endif
}

/**.......................................................................
 * Set the antenna id associated with this data frame
 */
void AntennaDataFrameCorba::setAnt(gcp::util::AntNum::Id antennaId)
{
  AntennaDataFrame::setAnt(antennaId);
#if DIR_USE_ANT_CORBA
  frame_.antennaId_ = antNum_.getIntId();
#endif
}

/**.......................................................................
 * Set the antenna id associated with this data frame
 */
void AntennaDataFrameCorba::setAnt(const gcp::util::AntNum& antNum)
{
  AntennaDataFrame::setAnt(antNum);
#if DIR_USE_ANT_CORBA
  frame_.antennaId_ = antNum_.getIntId();
#endif
}

#if DIR_USE_ANT_CORBA
/**.......................................................................
 * Base-class assignment operator
 */
void AntennaDataFrameCorba::operator=(DataFrame& frame)
{
  operator=((AntennaDataFrameCorba&)frame);
}

/**.......................................................................
 * Local assignment operator
 */
void AntennaDataFrameCorba::operator=(AntennaDataFrameCorba& frame)
{
  // First call the base-class assignment method
  
  DataFrame::operator=((DataFrame&)frame);
  
  // Now assign our members
  
  setAnt(frame.frame_.antennaId_);
  resize(frame.frame_.nvals_);
  frame_.lvals_ = frame.frame_.lvals_;
  lvals_        = frame.lvals_;
}

/**.......................................................................
 * Return a pointer to the raw data frame managed by this class.
 */
gcp::antenna::corba::DataFrame* AntennaDataFrameCorba::frame()
{
  return &frame_;
}
#endif

/**.......................................................................
 * Return a pointer to our internal data
 */
unsigned char* AntennaDataFrameCorba::data()
{
#if DIR_USE_ANT_CORBA
  for(unsigned idata=0; idata < frame_.nvals_; idata++)
    lvals_[idata] = frame_.lvals_[idata];
#endif

  return &lvals_[0];
}
