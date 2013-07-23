// $Id: BoloPixel.h,v 1.1.1.1 2009/07/06 23:57:23 eml Exp $

#ifndef GCP_RECEIVER_BOLOPIXEL_H
#define GCP_RECEIVER_BOLOPIXEL_H

/**
 * @file BoloPixel.h
 * 
 * Tagged: Sat Dec 16 14:14:00 PST 2006
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:23 $
 * 
 * @author Erik Leitch
 */
#include "gcp/receiver/specific/BoloDevice.h"

#include <vector>

namespace gcp {
  namespace receiver {

    class BoloPixel {
    public:

      /**
       * Constructor.
       */
      BoloPixel();

      /**
       * Copy Constructor.
       */
      BoloPixel(const BoloPixel& objToBeCopied);

      /**
       * Copy Constructor.
       */
      BoloPixel(BoloPixel& objToBeCopied);

      /**
       * Const Assignment Operator.
       */
      void operator=(const BoloPixel& objToBeAssigned);

      /**
       * Assignment Operator.
       */
      void operator=(BoloPixel& objToBeAssigned);

      /**
       * Destructor.
       */
      virtual ~BoloPixel();

      void setBoloId(std::string id);
      void setBoloReadoutChannel(std::string channel);
      void setSquidId(std::string id);
      void setSquidReadoutChannel(std::string channel);
      void setPixelIndex(unsigned number);
      void setPixelXY(std::vector<double>& xy);

      std::string getBoloName();
      std::string getSquidName();
      std::string getBoloReadoutChannel();
      std::string getSquidReadoutChannel();
      unsigned getPixelIndex() {return iPix_;}
      std::vector<double> getPixelXY() {return xy_;}

      /**
       * Output Operator.
       */
      friend std::ostream& operator<<(std::ostream& os, BoloPixel& obj);

    private:

      unsigned iPix_;    // The index associated with this pixel
      BoloDevice bolo_;  // The bolometer device associated with this pixel
      BoloDevice squid_; // The squid associated with this pixel
      std::vector<double> xy_;

    }; // End class BoloPixel

  } // End namespace receiver
} // End namespace gcp


#endif // End #ifndef GCP_RECEIVER_BOLOPIXEL_H
