#ifndef EXPSTUBDRIVETASK_H
#define EXPSTUBDRIVETASK_H

/**
 * @file SpecificDriveTask.h
 * 
 * Tagged: Thu Nov 13 16:53:53 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/GenericTask.h"
#include "gcp/antenna/control/specific/SpecificShare.h"
#include "gcp/antenna/control/specific/AntennaDriveMsg.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      
      class SpecificDriveTask : 
	public gcp::util::GenericTask<AntennaDriveMsg> {
	
	public:
	
	/**
	 * Constructor just initializes the shared object pointer to
	 * NULL.
	 */
	inline SpecificDriveTask() {
	  share_   = 0;
	};
	
	/**
	 * Make this virtual so that inheritors destructors are
	 * properly called even if they are upcast.
	 */
	inline virtual ~SpecificDriveTask() {};
	
	/**
	 * Public method to get a pointer to our shared object.
	 */
	inline SpecificShare* getShare();
	
	protected:
	
	SpecificShare* share_;
	
      }; // End class SpecificDriveTask
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
