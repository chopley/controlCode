#include "gcp/receiver/specific/BoloDevice.h"

#include<iostream>

using namespace std;

using namespace gcp::receiver;

/**.......................................................................
 * Constructor.
 */
BoloDevice::BoloDevice() {}

/**.......................................................................
 * Const Copy Constructor.
 */
BoloDevice::BoloDevice(const BoloDevice& objToBeCopied)
{
  *this = (BoloDevice&)objToBeCopied;
}

/**.......................................................................
 * Copy Constructor.
 */
BoloDevice::BoloDevice(BoloDevice& objToBeCopied)
{
  *this = objToBeCopied;
}

/**.......................................................................
 * Const Assignment Operator.
 */
void BoloDevice::operator=(const BoloDevice& objToBeAssigned)
{
  *this = (BoloDevice&)objToBeAssigned;
}

/**.......................................................................
 * Assignment Operator.
 */
void BoloDevice::operator=(BoloDevice& objToBeAssigned)
{
  id_ = objToBeAssigned.id_;
  readoutChannel_ = objToBeAssigned.readoutChannel_;
}

/**.......................................................................
 * Output Operator.
 */
std::ostream& gcp::receiver::operator<<(std::ostream& os, BoloDevice& obj)
{
  os << "id  = " << obj.id_ << ", channel = " << obj.readoutChannel_;
  return os;
}

/**.......................................................................
 * Destructor.
 */
BoloDevice::~BoloDevice() {}

void BoloDevice::setId(std::string id)
{
  id_ = id;
}

void BoloDevice::setReadoutChannel(std::string channel)
{
  readoutChannel_ = channel;
}

std::string BoloDevice::getId()
{
  return id_;
}

std::string BoloDevice::getReadoutChannel()
{
  return readoutChannel_;
}
