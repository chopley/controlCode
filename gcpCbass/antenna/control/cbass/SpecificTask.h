#ifndef GCP_ANTENNA_CONTROL_EXPSTUBTASK_H
#define GCP_ANTENNA_CONTROL_EXPSTUBTASK_H

/**
 * @file SpecificTask.h
 * 
 * Tagged: Thu Nov 13 16:53:54 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/specific/Directives.h"

#include "gcp/antenna/control/specific/SpecificShare.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      class SpecificTask {
	
      public:
	
	/**
	 * Constructor.
	 */
	SpecificTask();
	
	/**
	 * Make this virtual so that inheritors destructors are
	 * properly called even if they are upcast.
	 */
	virtual ~SpecificTask();
	
	/**
	 * Public method to get a pointer to our shared object.
	 */
	SpecificShare* getShare();
	
	/**
	 * The shared-memory object.
	 */
	SpecificShare* share_;
	
      }; // End class SpecificTask

    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
