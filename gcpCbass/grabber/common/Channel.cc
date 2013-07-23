#include "gcp/grabber/common/Channel.h"

#include "gcp/util/common/Exception.h"

#include<iostream>

using namespace std;

using namespace gcp::grabber;

/**.......................................................................
 * Constructor.
 */
Channel::Channel() {}


/**.......................................................................
 * Destructor.
 */
Channel::~Channel() {}

unsigned Channel::channelToInt(Channel::FgChannel chanMask) 
{
  switch (chanMask) {
  case Channel::CHAN0:
    return 0;
    break;
  case Channel::CHAN1:
    return 1;
    break;
  case Channel::CHAN2:
    return 2;
    break;
  case Channel::CHAN3:
    return 3;
    break;
  default:
    ThrowError("channel mask does not represent a single channel");
    break;
  }
  
};
      
Channel::FgChannel Channel::intToChannel(unsigned chanInt) 
{
  switch (chanInt) {
  case 0:
    return Channel::CHAN0;
    break;
  case 1:
    return Channel::CHAN1;
    break;
  case 2:
    return Channel::CHAN2;
    break;
  case 3:
    return Channel::CHAN3;
    break;
  default:
    ThrowError("channel: " << chanInt << " is not a valid channel");
    break;
  }
  
};

/**.......................................................................
 * Write the contents to an ostream
 */
ostream& 
gcp::grabber::operator<<(ostream& os, const Channel::FgChannel& chan)
{
  bool hasPrev=false;

  if(chan == Channel::NONE)
    os << "NONE";
  else if(chan == Channel::ALL)
    os << "ALL";
  else {
    
    if(chan & Channel::CHAN0) {
      os << "CHAN0";
      hasPrev = true;
    } 
    
    if(chan & Channel::CHAN1) {
      os << "CHAN1";
      if(hasPrev)
	os << "+";
      hasPrev = true;
    } 
    
    if(chan & Channel::CHAN2) {
      os << "CHAN2";
      if(hasPrev)
	os << "+";
    } 
    
    if(chan & Channel::CHAN3) {
      os << "CHAN3";
      if(hasPrev)
	os << "+";
    } 
  }

  return os;
}

bool Channel::isSingleChannel(Channel::FgChannel chan) 
{
  switch (chan) {
  case Channel::CHAN0:
  case Channel::CHAN1:
  case Channel::CHAN2:
  case Channel::CHAN3:
    return true;
    break;
  default:
    return false;
    break;
  }
};

std::vector<Channel::FgChannel> Channel::getChannels(Channel::FgChannel chanMask) 
{
  std::vector<Channel::FgChannel> channels;

  if(chanMask & CHAN0)
    channels.push_back(CHAN0);

  if(chanMask & CHAN1)
    channels.push_back(CHAN1);

  if(chanMask & CHAN2)
    channels.push_back(CHAN2);

  if(chanMask & CHAN3)
    channels.push_back(CHAN3);

  return channels;
}
