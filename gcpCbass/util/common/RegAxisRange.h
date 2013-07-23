#ifndef GCP_UTIL_REGAXISRANGE_H
#define GCP_UTIL_REGAXISRANGE_H

/**
 * @file RegAxisRange.h
 * 
 * Tagged: Sat Oct  2 00:14:18 UTC 2004
 * 
 * @author 
 */
#include "gcp/util/common/AxisRange.h"
#include "gcp/util/common/CoordRange.h"

#include "gcp/control/code/unix/libunix_src/common/genericregs.h"

#include <vector>

namespace gcp {
  namespace util {
    
    class RegDescription;
    
    /**
     * A class for iterating over slot ranges specified in a
     * CoordRange object
     */
    class RegAxisRange : public AxisRange {
    public:
      
      /**
       * Constructor.
       */
      RegAxisRange(RegDescription& reg, CoordRange& range);
      RegAxisRange(RegDescription& reg, CoordRange* range=0);
      RegAxisRange();
      RegAxisRange(RegMapBlock* block);
      
      void setTo(RegDescription& reg, CoordRange* range);
      
      /**
       * Destructor.
       */
      virtual ~RegAxisRange();
      
      /**
       * Return the current element
       */
      inline int currentSlot() {
	return iSlotOffset_ < 0 ? -1 : (int)currentElement() + iSlotOffset_;
      }
      
      /**
       * Return the (singleton) coordinate range corresponding to the
       * current element
       */
      CoordRange currentCoordRange();
      
      // Assignment operator

      void operator=(const RegAxisRange& obj);
      void operator=(RegAxisRange& obj);

      /**
       * Allows cout << RegAxisRange
       */
      friend std::ostream& operator<<(std::ostream& os, RegAxisRange& range);
      
    private:
      
      int iSlotOffset_;
      
    }; // End class RegAxisRange
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_REGAXISRANGE_H
