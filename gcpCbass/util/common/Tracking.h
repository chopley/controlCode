#ifndef GCP_UTIL_TRACKING_H
#define GCP_UTIL_TRACKING_H

/**
 * @file Tracking.h
 * 
 * Started: Wed Dec 17 19:50:04 UTC 2003
 * 
 * @author Erik Leitch
 */
namespace gcp {
  namespace util {
    
    /**
     * Class to enumerate the current telescope tracking type.
     */
    class Tracking {
    public:
      
      enum Type {
	TRACK_NONE  = 0x0,
	TRACK_POINT = 0x2,
	TRACK_PHASE = 0x4,
	TRACK_BOTH  = TRACK_POINT | TRACK_PHASE	
      };
      
      enum SlewType {
	TRACK_FIXED  = 0x1,
	TRACK_CENTER = 0x2,
      };

    }; // End class Tracking
    
  } // End namespace util
} // End namespace gcp


#endif
