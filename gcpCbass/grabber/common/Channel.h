// $Id: Channel.h,v 1.2 2010/03/04 21:17:26 sjcm Exp $

#ifndef GCP_GRABBER_CHANNEL_H
#define GCP_GRABBER_CHANNEL_H

/**
 * @file Channel.h
 * 
 * Tagged: Sun Jan 13 14:23:48 NZDT 2008
 * 
 * @version: $Revision: 1.2 $, $Date: 2010/03/04 21:17:26 $
 * 
 * @author SPT
 */

#include <iostream>
#include <vector>

namespace gcp {
  namespace grabber {

    class Channel {
    public:

      enum FgChannel {
	NONE  = 0x0,
	CHAN0 = 0x1,
	CHAN1 = 0x2,
	CHAN2 = 0x4,
	CHAN3 = 0x8,
	ALL   = CHAN0 | CHAN1 | CHAN2 | CHAN3
      };

      static const unsigned nChan_ = 4;

      friend std::ostream& operator<<(std::ostream& os, const FgChannel& chan);
      
      // Constructor.

      Channel();

      // Destructor.

      virtual ~Channel();

      // Convert from FgChannel to integer channel ID

      static unsigned channelToInt(FgChannel chanMask);

      // Convert from integer channel ID to FgChannel

      static FgChannel intToChannel(unsigned chanInt);

      static bool isSingleChannel(FgChannel chan);

      static std::vector<Channel::FgChannel> getChannels(Channel::FgChannel chanMask);

    }; // End class Channel

    std::ostream& operator<<(std::ostream& os, const Channel::FgChannel& chan);

  } // End namespace grabber
} // End namespace gcp



#endif // End #ifndef GCP_GRABBER_CHANNEL_H
