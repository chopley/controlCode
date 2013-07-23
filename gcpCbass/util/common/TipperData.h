#ifndef GCP_UTIL_TIPPERDATA_H
#define GCP_UTIL_TIPPERDATA_H

/**
 * @file TipperData.h
 * 
 * Tagged: Wed 01-Feb-06 15:41:19
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/NetStruct.h"

namespace gcp {
  namespace util {
    
    class TipperData : public NetStruct {
    public:

      /**
       * Constructor.
       */
      TipperData();
      
      /**
       * Destructor.
       */
      virtual ~TipperData();
      
      unsigned mjdDays_;
      unsigned mjdMs_;
      double filter_;
      double tHot_;
      double tWarm_;
      double tAmb_;
      double tChop_;
      double tInt_;
      double tSnork_;
      double tAtm_;
      double tau_;
      double tSpill_;
      double r_;
      double mse_;

    // Write the contents of this object to an ostream

    friend std::ostream& 
      gcp::util::operator<<(std::ostream& os, TipperData& data);

    std::string header();

    }; // End class TipperData

    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_TIPPERDATA_H
