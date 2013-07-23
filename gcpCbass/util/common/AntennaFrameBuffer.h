#ifndef GCP_UTIL_ANTENNAFRAMEBUFFER_H
#define GCP_UTIL_ANTENNAFRAMEBUFFER_H

/**
 * @file AntennaFrameBuffer.h
 * 
 * Tagged: Mon Mar 22 03:56:39 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/FrameBuffer.h"

namespace gcp {
  namespace util {
    
    class AntennaFrameBuffer : public gcp::util::FrameBuffer {
    public:
      
      /**
       * Constructor.
       */
      AntennaFrameBuffer(const gcp::util::AntNum& antNum,
			 unsigned int nFrame, 
			 bool archivedOnly=false);
      
      /**
       * Constructor.
       */
      AntennaFrameBuffer(const gcp::util::AntNum& antNum, 
			 bool archivedOnly=false);
      
      /**
       * Constructor.
       */
      AntennaFrameBuffer(gcp::util::AntNum* antNum, 
			 bool archivedOnly=false);
      
      /**
       * Destructor.
       */
      virtual ~AntennaFrameBuffer();
      
    private:
    }; // End class AntennaFrameBuffer
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_ANTENNAFRAMEBUFFER_H
