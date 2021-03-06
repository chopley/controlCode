#ifndef GCP_UTIL_AMP_H
#define GCP_UTIL_AMP_H

/**
 * @file Amp.h
 * 
 * Tagged: Tue Mar 30 08:46:08 PST 2004
 * 
 * @author Erik Leitch
 */
namespace gcp {
  namespace util {
    
    /**
     * A class for enumerating amplifiers
     */
    class Amp {
    public:
      
      enum Type {
	RF = 0x1,
	IF = 0x2
      };
      
      enum Stage {
	FIRST  = 0x1,
	SECOND = 0x2,
	THIRD  = 0x4,
	FOURTH = 0x8
      };
      
      enum Bias {
	GATE_VOLTAAGE = 0x1,
	DRAIN_VOLTAGE = 0x2,
	DRAIN_CURRENT = 0x4,
      };
    }; // End class Amp
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_AMP_H
