#ifndef GCP_UTIL_ANTENNADATAFRAME_H
#define GCP_UTIL_ANTENNADATAFRAME_H

/**
 * @file AntennaDataFrame.h
 * 
 * Tagged: Sun Mar 21 17:29:13 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/AntNum.h"

namespace gcp {
  namespace util {
    
    class AntennaDataFrame {
    public:
      
      /**
       * Constructor
       */
      AntennaDataFrame();
      
      /**
       * Constructor
       */
      AntennaDataFrame(const gcp::util::AntNum& antNum);
      
      /**
       * Destructor
       */
      virtual ~AntennaDataFrame();
      
      /**
       * Set the antenna id associated with this data frame.
       */
      virtual void setAnt(unsigned int);
      
      /**
       * Set the antenna id associated with this data frame.
       */
      virtual void setAnt(gcp::util::AntNum::Id antennaId);
      
      /**
       * Set by reference
       */
      virtual void setAnt(const gcp::util::AntNum& antNum);
      
      /**
       * Return the antenna id associated with this data frame.
       */
      AntNum getAnt();
      
      /**
       * Return the antenna id associated with this data frame.
       */
      unsigned getAntIntId();
      
      /**
       * Assignment operators
       */
      void operator=(AntennaDataFrame& frame);
      
      /**
       * Get a pointer to our internal data suitable for using as an
       * external network buffer
       */
      unsigned char* data();
      
    protected:
      
      gcp::util::AntNum antNum_;
      
    }; // End class AntennaDataFrame
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_ANTENNADATAFRAME_H
