#ifndef GCP_UTIL_AXISRANGE_H
#define GCP_UTIL_AXISRANGE_H

/**
 * @file AxisRange.h
 * 
 * Tagged: Tue Oct  5 13:18:11 PDT 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/CoordAxes.h"
#include "gcp/util/common/CoordRange.h"

#include "gcp/control/code/unix/libunix_src/common/genericregs.h"

namespace gcp {
  namespace util {
    
    class AxisRange {
    public:
      
      /**
       * Constructor.
       */
      AxisRange(CoordAxes axes, CoordRange range);
      AxisRange(CoordAxes& axes, CoordRange* range=0);
      AxisRange(CoordAxes* axes, CoordRange* range=0);
      AxisRange(RegMapBlock* block, CoordRange* range=0);
      
      /**
       * Constructor for a simplistic axis conisting of nEl
       * consecutive elements
       */
      AxisRange(unsigned nEl);
      AxisRange();
      
      /**
       * Set the axis range
       */
      void setTo(CoordAxes* axes, CoordRange* range);
      void setToDc(CoordAxes* axes, CoordRange* range);
      void setTo(unsigned nEl);
      
      /**
       * Destructor.
       */
      virtual ~AxisRange();
      
      /**
       * Return the current element
       */
      inline unsigned currentElement() {
	return iElCurrent_;
      }
      
      /**
       * Return the current iterator
       */
      inline unsigned currentIterator() {
	return iter_;
      }
      
      void operator=(AxisRange& obj);

      /**.......................................................................
       * Prefix increment operator
       */
      const AxisRange& operator++()
      {
        // If we are done, just return silently
      
        if(!isEnd()) {
      
          // Increment the monotonic iterator
      
          iter_++;
      
          // If we just did the last element of the current range, advance
          // to the next range
      
          if(iElCurrent_ == (*iRange_).stop()) {
            
            // If we are on the last range just stop
      
            if(iRange_ < ranges_.end()) {
      	iRange_++;
      	
      	// And set the new element pointing to the beginning of this
      	// range
      
      	if(!isEnd())
      	  iElCurrent_ = (*iRange_).start();
            }
      
          } else // Else increment to the next slot
            iElCurrent_++;
        }
      
        return *this;
      }

      /**
       * Reset all iterators
       */
      void reset();
      
      /**
       * Return true if we are at the end of our range.
       */
      bool isEnd();
      
      /**
       * Allows cout << AxisRange
       */
      friend std::ostream& operator<<(std::ostream& os, AxisRange& range);
      
      /**
       * Return the current coordinate
       */
      Coord currentCoord();
      
      unsigned nEl();
      
    private:
      
      unsigned iter_;
      unsigned iElCurrent_;
      std::vector<Range<unsigned> > ranges_;
      std::vector<Range<unsigned> >::iterator iRange_;
      CoordAxes axes_;
      unsigned nEl_;
      
    }; // End class AxisRange
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_AXISRANGE_H
