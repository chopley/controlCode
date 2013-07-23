#ifndef GCP_RECEIVER_BOLOMETERDCDATAFRAMEMANAGER_H
#define GCP_RECEIVER_BOLOMETERDCDATAFRAMEMANAGER_H

/**
 * @file SquidDataFrameManager.h
 * 
 * Tagged: Wed Mar 31 09:59:54 PST 2004
 * 
 * @author Erik Leitch
 */

// C header files from the array control code

#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"

#include "gcp/util/common/BoardDataFrameManager.h"
#include "gcp/util/common/TimeVal.h"

#include "Utilities/ReadoutAddress.h"

#include <map>
#include <vector>

namespace gcp {
  namespace receiver {
      
      class DataFrame;
      
      class SquidDataFrameManager : 
	public gcp::util::BoardDataFrameManager {

      public:
	
	/**
	 * Copy constructor
	 */
	SquidDataFrameManager();
	
	/**
	 * Destructor.
	 */
	virtual ~SquidDataFrameManager();
	
	void setMjd(gcp::util::RegDate& date);

	void bufferVolts(float val, unsigned iSquid);

	void bufferId(std::string& id, unsigned iSquid);

	void setSubSampling(unsigned sampling);

	void setFilterNtap(unsigned ntaps);

      private:      
	
	RegMapBlock* voltsPtr_;
	float volts_[NUM_SQUIDS];

	RegMapBlock* idPtr_;
	unsigned char ids_[NUM_SQUIDS * DIO_ID_LEN];
	
      }; // End class SquidDataFrameManager
      
  } // End namespace receiver
}; // End namespace gcp




#endif // End #ifndef GCP_RECEIVER_BOLOMETERDCDATAFRAMEMANAGER_H
