#include "gcp/receiver/specific/BoloPixel.h"
#include "gcp/util/common/Exception.h"

#include<iostream>

using namespace std;

using namespace gcp::receiver;

/**.......................................................................
 * Constructor.
 */
BoloPixel::BoloPixel() {}

/**.......................................................................
 * Const Copy Constructor.
 */
BoloPixel::BoloPixel(const BoloPixel& objToBeCopied)
{
  *this = (BoloPixel&)objToBeCopied;
};

/**.......................................................................
 * Copy Constructor.
 */
BoloPixel::BoloPixel(BoloPixel& objToBeCopied)
{
  *this = objToBeCopied;
};

/**.......................................................................
 * Const Assignment Operator.
 */
void BoloPixel::operator=(const BoloPixel& objToBeAssigned)
{
  *this = (BoloPixel&)objToBeAssigned;
};

/**.......................................................................
 * Assignment Operator.
 */
void BoloPixel::operator=(BoloPixel& objToBeAssigned)
{
  bolo_  = objToBeAssigned.bolo_;
  squid_ = objToBeAssigned.squid_;
  iPix_  = objToBeAssigned.iPix_;
  xy_    = objToBeAssigned.xy_;
};

/**.......................................................................
 * Destructor.
 */
BoloPixel::~BoloPixel() {}

void BoloPixel::setBoloId(std::string id)
{
  bolo_.setId(id);
}

void BoloPixel::setBoloReadoutChannel(std::string channel)
{
  bolo_.setReadoutChannel(channel);
}

void BoloPixel::setSquidId(std::string id) 
{
  squid_.setId(id);
}

void BoloPixel::setSquidReadoutChannel(std::string channel)
{
  squid_.setReadoutChannel(channel);
}

void BoloPixel::setPixelIndex(unsigned number)
{
  iPix_ = number;
}

void BoloPixel::setPixelXY(std::vector<double>& xy)
{
  if(xy.size() != 2)
    ThrowError("Received xy array with invalid size: " << xy.size());

  xy_ = xy;
}

/**.......................................................................
 * Output Operator.
 */
std::ostream& gcp::receiver::operator<<(std::ostream& os, BoloPixel& obj)
{
  COUT("Inside output operator");

  os << "Bolo  = " << obj.bolo_ << " Squid = " << obj.squid_ 
     << " iPix = " << obj.iPix_;

  if(obj.xy_.size() != 2) {
    os << " pixel xy are not assigned (xy_.size() = " << obj.xy_.size() << ")";
  } else {
    os << " x = " << obj.xy_[0]
       << " y = " << obj.xy_[1];
  }

  return os;
};

std::string BoloPixel::getBoloName()
{
  return bolo_.getId();
}

std::string BoloPixel::getSquidName()
{
  return squid_.getId();
}


std::string BoloPixel::getBoloReadoutChannel()
{
  return bolo_.getReadoutChannel();
}

std::string BoloPixel::getSquidReadoutChannel()
{
  return squid_.getReadoutChannel();
}
