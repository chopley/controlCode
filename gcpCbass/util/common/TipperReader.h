#ifndef GCP_UTIL_TIPPERREADER_H
#define GCP_UTIL_TIPPERREADER_H

/**
 * @file TipperReader.h
 * 
 * Tagged: Wed 01-Feb-06 15:36:17
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/TipperData.h"

namespace gcp {
  namespace util {
    
    class TipperReader {
    public:
      
      /**
       * Constructor.
       */
      TipperReader();
      
      /**
       * Destructor.
       */
      virtual ~TipperReader();
      
    protected:

      TipperData data_;

      void getMostRecentData();

    }; // End class TipperReader
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_TIPPERREADER_H
