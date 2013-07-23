// $Id: Pressure.h,v 1.4 2010/03/04 21:17:27 sjcm Exp $

#ifndef GCP_UTIL_PRESSURE_H
#define GCP_UTIL_PRESSURE_H

/**
 * @file Pressure.h
 * 
 * Tagged: Fri Nov 14 17:07:29 PST 2008
 * 
 * @version: $Revision: 1.4 $, $Date: 2010/03/04 21:17:27 $
 * 
 * @author username: Command not found.
 */
#include <sstream>

#include "gcp/util/common/ConformableQuantity.h"

namespace gcp {
  namespace util {

    class Pressure : public ConformableQuantity {
    public:
      
      class InchesHg {};
      class MilliBar {};
      class Torr {};

      /**
       * Constructor.
       */
      Pressure();
      Pressure(const InchesHg& unit, double inchesHg);
      Pressure(const MilliBar& unit, double mBar);
      Pressure(const Torr& unit,     double torr);

      /**
       * Destructor.
       */
      virtual ~Pressure();

      // Scale factors used by this class

      static const double mBarPerInHg_;

      void setMilliBar(double mBar);
      void setInchesHg(double inchesHg);
      void setInHg(double inchesHg);
      void setTorr(double torr);

      double milliBar();
      double inchesHg();
      double inHg();
      double torr();

      friend std::ostream& operator<<(std::ostream& os, Pressure& pressure);

    private:

      double mBar_;

    }; // End class Pressure

    std::ostream& operator<<(std::ostream& os, Pressure& pressure);

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_PRESSURE_H
