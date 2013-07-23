#ifndef GCP_UTIL_FRAMEFLAGS_H
#define GCP_UTIL_FRAMEFLAGS_H

/**
 * @file FrameFlags.h
 * 
 * Tagged: Wed Mar 23 10:41:08 PST 2005
 * 
 * @author GCP data acquisition
 */
namespace gcp {
  namespace util {
    
    class FrameFlags {
    public:
      
      /*
       * Enumerate states used to indicate if data from a given
       * frame were received/not received
       */
      enum {
	NOT_RECEIVED = 0x1,
	RECEIVED     = 0x2
      };
      
    }; // End class FrameFlags
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_FRAMEFLAGS_H
