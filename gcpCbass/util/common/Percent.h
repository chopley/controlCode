// $Id: Percent.h,v 1.1 2009/08/17 21:21:07 eml Exp $

#ifndef GCP_UTIL_PERCENT_H
#define GCP_UTIL_PERCENT_H

/**
 * @file Percent.h
 * 
 * Tagged: Mon Aug 17 11:01:48 PDT 2009
 * 
 * @version: $Revision: 1.1 $, $Date: 2009/08/17 21:21:07 $
 * 
 * @author tcsh: username: Command not found.
 */
#include "gcp/util/common/ConformableQuantity.h"

namespace gcp {
  namespace util {

    class Percent : public ConformableQuantity {
    public:

      class Max1 {};
      class Max100 {};

      /**
       * Constructor.
       */
      Percent();
      Percent(const Max1& unit, double percent);
      Percent(const Max100& unit, double percent);

      /**
       * Destructor.
       */
      virtual ~Percent();

      void setPercentMax1(double percent);
      void setPercentMax100(double percent);

      double percentMax1();
      double percentMax100();

    private:

      double percentMax1_;

    }; // End class Percent

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_PERCENT_H
