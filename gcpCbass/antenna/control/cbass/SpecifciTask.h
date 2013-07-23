#ifndef GCP_ANTENNA_CONTROL_SZATASK_H
#define GCP_ANTENNA_CONTROL_SZATASK_H

/**
 * @file SzaTask.h
 * 
 * Tagged: Thu Nov 13 16:53:54 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/specific/Directives.h"

#include "gcp/antenna/control/specific/SzaShare.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      class SzaTask {
	
      public:
	
	/**
	 * Constructor.
	 */
	SzaTask();
	
	/**
	 * Make this virtual so that inheritors destructors are
	 * properly called even if they are upcast.
	 */
	virtual ~SzaTask();
	
	/**
	 * Public method to get a pointer to our shared object.
	 */
	SzaShare* getShare();
	
	protected:
	
	/**
	 * The shared-memory object.
	 */
	SzaShare* share_;
	
      }; // End class SzaTask

    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
