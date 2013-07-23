#include "gcp/receiver/specific/BoloDoubleBuffer.h"

#include "gcp/receiver/specific/BolometerDataFrameManager.h"

#include<iostream>

using namespace std;

using namespace gcp::receiver;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
BoloDoubleBuffer::BoloDoubleBuffer() 
{
  BolometerDataFrameManager* bdfm = 0;

  // Initialize all data to NAN.  This will cause channels not present
  // to default to the invalid flag

  bdfm = new BolometerDataFrameManager();
  bdfm->writeReg("bolometers", "adc", NAN);  
  buf1_.data_ = (void*) bdfm;

  bdfm = new BolometerDataFrameManager();
  bdfm->writeReg("bolometers", "adc", NAN);  
  buf2_.data_ = (void*) bdfm;
}

/**.......................................................................
 * Destructor.
 */
BoloDoubleBuffer::~BoloDoubleBuffer() 
{
  if(buf1_.data_)
    delete (BolometerDataFrameManager*) buf1_.data_;

  if(buf2_.data_)
    delete (BolometerDataFrameManager*) buf2_.data_;
}
