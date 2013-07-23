#ifndef GCP_UTIL_ANTENNADATAFRAMENORMAL_H
#define GCP_UTIL_ANTENNADATAFRAMENORMAL_H

/**
 * @file AntennaDataFrameNormal.h
 * 
 * Tagged: Sun Mar 21 16:58:29 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/AntennaDataFrame.h"
#include "gcp/util/common/DataFrameNormal.h"

namespace gcp {
  namespace util {
    
    /**
     * This is just a normal data frame, with an additional member
     * which specifies the antenna.
     */
    class AntennaDataFrameNormal : 
      public gcp::util::AntennaDataFrame,
      public gcp::util::DataFrameNormal {
      
      public:
      
      /**
       * Constructor
       */
      AntennaDataFrameNormal(unsigned int nBuffer);
      
      /**
       * Assignment operators
       */
      void operator=(gcp::util::DataFrameNormal& frame);
      void operator=(AntennaDataFrameNormal& frame);
      
    }; // End class AntennaDataFrameNormal
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_ANTENNADATAFRAMENORMAL_H
