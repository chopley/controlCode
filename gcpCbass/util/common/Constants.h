#ifndef GCP_UTIL_CONSTANTS_H
#define GCP_UTIL_CONSTANTS_H

/**
 * @file Constants.h
 * 
 * Tagged: Tue Aug 10 13:17:59 PDT 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Speed.h"
#include "gcp/util/common/Temperature.h"

namespace gcp {
  namespace util {
    
    class Constants {
    public:
      
      static Temperature Tcmb_;
      static const double hPlanckCgs_;
      static const double kBoltzCgs_;
      static const double JyPerCgs_;
      static const double sigmaTCgs_;
      static const double electronMassCgs_;
      static const double protonMassCgs_;
	
      static Speed lightSpeed_;

      /**
       * Constructor.
       */
      Constants();
      
      /**
       * Destructor.
       */
      virtual ~Constants();
      
      virtual double cgs() {
	return cgs_;
      }

      virtual double si() {
	return si_;
      }

    protected:

      void setGgs(double cgs) {
	cgs = cgs_;
      }

      void setSi(double si) {
	si = si_;
      }
      
    private:

      double cgs_;
      double si_;

    }; // End class Constants
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_CONSTANTS_H
