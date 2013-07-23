#ifndef GCP_UTIL_POINTINGTELESCOPES_H
#define GCP_UTIL_POINTINGTELESCOPES_H

/**
 * @file PointingTelescopes.h
 * 
 * Tagged: Fri Jan 18 16:06:39 NZDT 2008
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:26 $
 * 
 * @author SPT
 */

#include <iostream>
#include <map>

#include "gcp/grabber/common/Channel.h"

#include "gcp/util/common/Mutex.h"

namespace gcp {
  namespace util {

    class PointingTelescopes {
    public:

      enum Ptel {
	PTEL_NONE       = 0x0,
	PTEL_ZERO       = 0x1,
	PTEL_ONE        = 0x2,
	PTEL_TWO        = 0x4,
	PTEL_ALL        = PTEL_ZERO | PTEL_ONE | PTEL_TWO
      };

      static const unsigned short nPtel_ = 3;

      // Constructor.

      PointingTelescopes();

      // Output Operator.

      friend std::ostream& operator<<(std::ostream& os, PointingTelescopes& obj);

      // Destructor.

      virtual ~PointingTelescopes();

      static unsigned ptelToInt(Ptel ptel);
      static Ptel intToPtel(unsigned iPtel);

      // Return the pointing telescope that corresponds to the passed
      // frame grabber channel ID

      static Ptel getPtels(gcp::grabber::Channel::FgChannel channel);
      static Ptel getSinglePtel(gcp::grabber::Channel::FgChannel channel);

      // Return the pointing telescope that corresponds to the 
      // frame grabber channel number (integer from 0-2)

      static Ptel getSinglePtel(unsigned channel);

      // Return the frame grabber channel id that corresponds to the
      // passed pointing telescope

      static gcp::grabber::Channel::FgChannel getFgChannels(Ptel ptelMask);

      static gcp::grabber::Channel::FgChannel getSingleFgChannel(Ptel ptel);

      // Return the integer frame grabber channel number that
      // corresponds to the passed pointing telescope

      static unsigned getSingleIntChannel(Ptel ptel);

      // Set up an asoociation between a pointing telescope and a
      // frame grabber channel

      static void assignFgChannel(Ptel ptel, 
				  gcp::grabber::Channel::FgChannel chan);

      // Static utility functions

      static bool isSinglePtel(Ptel ptel);

      static std::vector<Ptel> getPtels(Ptel ptelMask);

      friend std::ostream& 
	gcp::util::operator<<(std::ostream& os, 
				  PointingTelescopes::Ptel& ptel);

      static void setDefaultFgChannels(gcp::grabber::Channel::FgChannel channels);
      static void setDefaultPtels(Ptel ptels);

      static gcp::grabber::Channel::FgChannel getDefaultFgChannels();

      static PointingTelescopes::Ptel getDefaultPtels();
      
    private:

      static Mutex guard_;

      // A default channel mask to use

      static gcp::grabber::Channel::FgChannel defaultChannels_;
      
      // A map of ptel - channel associations

      static std::map<PointingTelescopes::Ptel, 
	gcp::grabber::Channel::FgChannel> channels_;

      // A map of channel - ptel associations

      static std::map<gcp::grabber::Channel::FgChannel, 
	PointingTelescopes::Ptel> ptels_;

    }; // End class PointingTelescopes

  } // End namespace util
} // End namespace gcp

#endif
