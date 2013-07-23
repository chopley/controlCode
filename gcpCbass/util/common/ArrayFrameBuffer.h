#ifndef GCP_TRANSLATOR_ARRAYFRAMEBUFFER_H
#define GCP_TRANSLATOR_ARRAYFRAMEBUFFER_H

/**
 * @file ArrayFrameBuffer.h
 * 
 * Tagged: Tue Mar 23 19:01:49 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/FrameBuffer.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/control/code/unix/libunix_src/common/scanner.h"

namespace gcp {
  namespace util {
    
    class ArrayFrameBuffer : public gcp::util::FrameBuffer {
    public:
      
      /**
       * Constructor.
       */
      ArrayFrameBuffer(unsigned int nFrame = SCAN_MAX_FRAME, 
		       bool archivedOnly=false);
      
      /**
       * Destructor.
       */
      virtual ~ArrayFrameBuffer();
      
    private:
      
      gcp::util::TimeVal timeVal;
      
    }; // End class ArrayFrameBuffer
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_TRANSLATOR_ARRAYFRAMEBUFFER_H
