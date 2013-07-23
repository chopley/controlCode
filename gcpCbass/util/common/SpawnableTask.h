// $Id: SpawnableTask.h,v 1.2 2009/08/13 21:34:51 eml Exp $

#ifndef GCP_UTIL_SPAWNABLETASK_H
#define GCP_UTIL_SPAWNABLETASK_H

/**
 * @file SpawnableTask.h
 * 
 * Tagged: Fri Jan 26 16:49:57 NZDT 2007
 * 
 * @version: $Revision: 1.2 $, $Date: 2009/08/13 21:34:51 $
 * 
 * @author Erik Leitch
 */

#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/RunnableTask.h"

#include <iostream>

namespace gcp {
  namespace util {

    /**.......................................................................
     * Define a template class for an object which can run in its own
     * thread.  This inherits the message queue mechanism from
     * GenericTask for communciation with this thread, and spawnable
     * capabilities from Runnable
     */
    template <class Msg>
      class SpawnableTask : public gcp::util::GenericTask<Msg>,
      public RunnableTask {

      public:

      /**
       * Constructor.  If spawn==true, then a call to spawn() will
       * start this thread
       */
      SpawnableTask(bool spawn) : RunnableTask(spawn) {}

      /**
       * Destructor.
       */
      virtual ~SpawnableTask() {};

      protected:
      
      // All the work in this class is done by processMsg().  
      //
      // Inheritors need only define what this function does for
      // different message types, and the rest will run itself.

      virtual void processMsg(Msg* msg) {};

      void run() {
	try {
	  GenericTask<Msg>::run();
	} catch(Exception& err) {
	  COUT("Caught an exception: " << err.what());
	} catch(...) {
	  COUT("Caught an exception");
	}
      }

    }; // End class SpawnableTask

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_SPAWNABLETASK_H
