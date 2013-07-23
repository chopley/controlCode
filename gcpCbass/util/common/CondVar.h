#ifndef GCP_UTIL_CONDVAR_H
#define GCP_UTIL_CONDVAR_H

/**
 * @file CondVar.h
 * 
 * Tagged: Thu Oct 18 13:20:31 PDT 2007
 * 
 * @author GCP data acquisition
 */
#include "gcp/util/common/Mutex.h"

#include <pthread.h>

namespace gcp {
  namespace util {
    
    class CondVar {
    public:
      
      /**
       * Constructor.
       */
      CondVar();
      
      /**
       * Destructor.
       */
      virtual ~CondVar();
      
      void wait();
      void broadcast();

    private:

      Mutex mutex_;
      pthread_cond_t cond_;
      bool condVarIsReady_;

    }; // End class CondVar
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_CONDVAR_H
