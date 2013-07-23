#ifndef GCP_UTIL_ARRAYREGMAPDATAFRAMEMANAGER_H
#define GCP_UTIL_ARRAYREGMAPDATAFRAMEMANAGER_H

/**
 * @file ArrayRegMapDataFrameManager.h
 * 
 * Tagged: Tue Oct 26 10:52:37 PDT 2004
 * 
 * @author GCP data acquisition
 */
// C header files from the array control code

#include "gcp/control/code/unix/libunix_src/common/genericregs.h"

#include "gcp/util/common/RegMapDataFrameManager.h"
#include "gcp/util/common/TimeVal.h"

namespace gcp {
  namespace util {
    
    class ArrayRegMapDataFrameManager : public RegMapDataFrameManager {
    public:
      
      /**
       * Constructor.
       */
      ArrayRegMapDataFrameManager(bool archivedOnly = false, 
				  DataFrame* dataFrame = 0);
      
      /**
       * Destructor.
       */
      virtual ~ArrayRegMapDataFrameManager();
      
      void initialize(bool archivedOnly = false, DataFrame* dataFrame = 0);
      
    private:
    }; // End class ArrayRegMapDataFrameManager
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_ARRAYREGMAPDATAFRAMEMANAGER_H
