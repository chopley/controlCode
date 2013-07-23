#include "gcp/util/common/Exception.h"
#include "gcp/util/common/PointingTelescopes.h"

#include<iostream>

using namespace std;

using namespace gcp::util;

gcp::grabber::Channel::FgChannel PointingTelescopes::defaultChannels_ = 
gcp::grabber::Channel::NONE;

std::map<PointingTelescopes::Ptel, gcp::grabber::Channel::FgChannel>
PointingTelescopes::channels_;

std::map<gcp::grabber::Channel::FgChannel, PointingTelescopes::Ptel>
PointingTelescopes::ptels_;

/**.......................................................................
 * Constructor.
 */
PointingTelescopes::PointingTelescopes() {}

/**.......................................................................
 * Output Operator.
 */
std::ostream& gcp::util::operator<<(std::ostream& os, PointingTelescopes::Ptel& ptel)
{
  bool hasPrev=false;

  if(ptel == PointingTelescopes::PTEL_NONE)
    os << "NONE";
  else if(ptel == PointingTelescopes::PTEL_ALL)
    os << "ALL";
  else {

    if(ptel & PointingTelescopes::PTEL_ZERO) {
      os << "PTEL_ZERO";
      hasPrev = true;
    } 
    
    if(ptel & PointingTelescopes::PTEL_ONE) {
      os << "PTEL_ONE";
      if(hasPrev)
	os << "+";
      hasPrev = true;
    } 
    
    if(ptel & PointingTelescopes::PTEL_TWO) {
      os << "PTEL_TWO";
      if(hasPrev)
	os << "+";
      hasPrev = true;
    } 
    
  }

  return os;
};

/**.......................................................................
 * Destructor.
 */
PointingTelescopes::~PointingTelescopes() {}

/**.......................................................................
 * Return the pointing telescopes that corresponds to the passed
 * frame grabber channel id mask
 */
PointingTelescopes::Ptel 
PointingTelescopes::getPtels(gcp::grabber::Channel::FgChannel chanMask)
{
  unsigned ptelMask = (unsigned)PointingTelescopes::PTEL_NONE;

  std::vector<gcp::grabber::Channel::FgChannel> channels = 
    gcp::grabber::Channel::getChannels(chanMask);

  for(unsigned i=0; i < channels.size(); i++) {
    ptelMask |= (unsigned)getSinglePtel(channels[i]);
  }

  return (Ptel)ptelMask;
}

/**.......................................................................
 * Return the pointing telescope that corresponds to the passed
 * frame grabber channel ID
 */
PointingTelescopes::Ptel 
PointingTelescopes::getSinglePtel(gcp::grabber::Channel::FgChannel channel)
{
  std::map<gcp::grabber::Channel::FgChannel, Ptel>::iterator slot;

  slot = ptels_.find(channel);
  
  if(slot != ptels_.end())
    return slot->second;

  ThrowSimpleError("No pointing telescope has been specified for channel id: " << channel);  
}

/**.......................................................................
 * Return the pointing telescope that corresponds to the 
 * frame grabber channel number (integer from 0-2)
 */
PointingTelescopes::Ptel 
PointingTelescopes::getSinglePtel(unsigned channel)
{
  return getSinglePtel(gcp::grabber::Channel::intToChannel(channel));
}

/**.......................................................................
 * Return the pointing telescopes that corresponds to the passed
 * frame grabber channel id mask
 */
gcp::grabber::Channel::FgChannel
PointingTelescopes::getFgChannels(Ptel ptelMask)
{
  unsigned chanMask = (unsigned)gcp::grabber::Channel::NONE;

  std::vector<Ptel> ptels = getPtels(ptelMask);

  for(unsigned i=0; i < ptels.size(); i++) {
    chanMask |= (unsigned)getSingleFgChannel(ptels[i]);
  }

  return (gcp::grabber::Channel::FgChannel)chanMask;
}


/**.......................................................................
 * Return the frame grabber channel id that corresponds to the
 * passed pointing telescope
 */
gcp::grabber::Channel::FgChannel 
PointingTelescopes::getSingleFgChannel(Ptel ptel)
{
  std::map<Ptel, gcp::grabber::Channel::FgChannel>::iterator slot;

  slot = channels_.find(ptel);
  
  if(slot != channels_.end())
    return slot->second;

  ThrowSimpleError("No channel has been specified for pointing telescope id: " 
		   << ptel << std::endl
		   << "Use: assignFrameGrabberChannel() to assign a telescope id to a frame grabber channel");  
}

/**.......................................................................
 * Return the integer frame grabber channel number that
 * corresponds to the passed pointing telescope
 */
unsigned PointingTelescopes::getSingleIntChannel(Ptel ptel)
{
  return gcp::grabber::Channel::channelToInt(getSingleFgChannel(ptel));
}

/**.......................................................................
 * Set up an asoociation between a pointing telescope and a
 * frame grabber channel
 */
void PointingTelescopes::assignFgChannel(Ptel ptel, 
					 gcp::grabber::Channel::FgChannel chan)
{
  if(!gcp::grabber::Channel::isSingleChannel(chan))
    ThrowError("Channel: " << chan << " does not represent a single channel");

  if(!PointingTelescopes::isSinglePtel(ptel))
    ThrowError("Ptel: " << ptel << " does not represent a single pointing telescope");

  // Make the associations in our maps

  std::map<gcp::grabber::Channel::FgChannel, Ptel>::iterator chanSlot;
  std::map<Ptel, gcp::grabber::Channel::FgChannel>::iterator ptelSlot;

  // See if this channel is already associated with a pointing
  // telescope

  ptelSlot = channels_.find(ptel);
  if(ptelSlot != channels_.end()) {
    ptels_.erase(ptelSlot->second);
  }

  // See if this telescope is already associated with a channel

  chanSlot = ptels_.find(chan);
  if(chanSlot != ptels_.end()) {
    channels_.erase(chanSlot->second);
  }

  // Now set up the new association

  channels_[ptel] = chan;
  ptels_[chan]    = ptel;
}

bool PointingTelescopes::isSinglePtel(Ptel ptel)
{
  switch (ptel) {
  case PointingTelescopes::PTEL_ZERO:
  case PointingTelescopes::PTEL_ONE:
  case PointingTelescopes::PTEL_TWO:
    return true;
    break;
  default:
    return false;
    break;
  }
}

std::vector<PointingTelescopes::Ptel> PointingTelescopes::getPtels(PointingTelescopes::Ptel ptelMask) 
{
  std::vector<PointingTelescopes::Ptel> ptels;

  if(ptelMask & PointingTelescopes::PTEL_ZERO)
    ptels.push_back(PointingTelescopes::PTEL_ZERO);

  if(ptelMask & PointingTelescopes::PTEL_ONE)
    ptels.push_back(PointingTelescopes::PTEL_ONE);

  if(ptelMask & PointingTelescopes::PTEL_TWO)
    ptels.push_back(PointingTelescopes::PTEL_TWO);

  return ptels;
}

void PointingTelescopes::
setDefaultFgChannels(gcp::grabber::Channel::FgChannel channelMask)
{
  defaultChannels_ = channelMask;
}

gcp::grabber::Channel::FgChannel 
PointingTelescopes::getDefaultFgChannels()
{
  if(defaultChannels_ == gcp::grabber::Channel::NONE) {
    ThrowSimpleError("No default frame grabber channel has been specified" 
		     << std::endl << std::endl
		     << "(You must specify a channel, or use setDefaultFrameGrabberChannel() to set a default)");
  }

  return defaultChannels_;
}

void PointingTelescopes::
setDefaultPtels(Ptel ptels)
{
  gcp::grabber::Channel::FgChannel chanMask = getFgChannels(ptels);
  setDefaultFgChannels(chanMask);
}

PointingTelescopes::Ptel 
PointingTelescopes::getDefaultPtels()
{
  if(defaultChannels_ == gcp::grabber::Channel::NONE) {
    ThrowSimpleError("No default pointing telescope has been specified");
  }

  return getPtels(defaultChannels_);
}

unsigned PointingTelescopes::ptelToInt(Ptel ptel)
{
  switch (ptel) {
  case PointingTelescopes::PTEL_ZERO:
    return 0;
    break;
  case PointingTelescopes::PTEL_ONE:
    return 1;
    break;
  case PointingTelescopes::PTEL_TWO:
    return 2;
    break;
  default:
    ThrowError("Pointing telescope id: " << ptel 
	       << " doesn't represent a valid single telescope");
    break;
  }
}

PointingTelescopes::Ptel PointingTelescopes::intToPtel(unsigned iPtel)
{
  switch (iPtel) {
  case 0:
    return PointingTelescopes::PTEL_ZERO;
    break;
  case 1:
    return PointingTelescopes::PTEL_ONE;
    break;
  case 2:
    return PointingTelescopes::PTEL_TWO;
    break;
  default:
    ThrowError("Pointing telescope index: " << iPtel
	       << " doesn't represent a valid single telescope");
    break;
  }
}
