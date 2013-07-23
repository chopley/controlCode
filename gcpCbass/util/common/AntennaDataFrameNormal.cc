#define __FILEPATH__ "util/common/AntennaDataFrameNormal.cc"

#include "gcp/util/common/AntennaDataFrameNormal.h"

using namespace gcp::util;

/**.......................................................................
 * Constructor with buffer initialization
 */
AntennaDataFrameNormal::AntennaDataFrameNormal(unsigned int nBuffer) :
  DataFrameNormal(nBuffer) {};

/**.......................................................................
 * Base-class assignment operator
 */
void AntennaDataFrameNormal::operator=(DataFrameNormal& frame)
{
  // Just call our local assignment operator
  
  operator=((AntennaDataFrameNormal&)frame);
}

/**.......................................................................
 * Local assignment operator.
 */
void AntennaDataFrameNormal::operator=(AntennaDataFrameNormal& frame)
{
  // First call the base-class DataFrame assignment method
  
  DataFrameNormal::operator=((DataFrameNormal&)frame);
  
  // Now call the base class AntennaDataFrame assignment operator
  
  AntennaDataFrame::operator=((AntennaDataFrame&)frame);
}
