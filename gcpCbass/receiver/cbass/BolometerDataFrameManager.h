#ifndef GCP_RECEIVER_BOLOMETERDATAFRAMEMANAGER_H
#define GCP_RECEIVER_BOLOMETERDATAFRAMEMANAGER_H

/**
 * @file BolometerDataFrameManager.h
 * 
 * Tagged: Wed Mar 31 09:59:54 PST 2004
 * 
 * @author Erik Leitch
 */

// C header files from the array control code

#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"
#include "gcp/util/common/RegMapDataFrameManager.h"

#include "Utilities/ReadoutAddress.h"

#include <map>
#include <vector>

namespace gcp {
  namespace receiver {
      
      class DataFrame;
      
      class BolometerDataFrameManager : 
	public gcp::util::RegMapDataFrameManager {

      public:
	
	/**
	 * Copy constructor
	 */
	BolometerDataFrameManager(BolometerDataFrameManager& fm);
	
	/**
	 * Constructor with initialization from a DataFrame object.
	 */
	BolometerDataFrameManager(bool archivedOnly=false, 
				   gcp::util::DataFrame* frame=0);
	
	/**
	 * Destructor.
	 */
	virtual ~BolometerDataFrameManager();
	
	/**
	 * Initialize this object.
	 */
	void initialize(bool archivedOnly=false, 
			gcp::util::DataFrame* frame = 0);
	
      }; // End class BolometerDataFrameManager
      
  } // End namespace receiver
}; // End namespace gcp




#endif // End #ifndef GCP_RECEIVER_BOLOMETERDATAFRAMEMANAGER_H
