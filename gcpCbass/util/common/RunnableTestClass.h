// $Id: RunnableTestClass.h,v 1.1.1.1 2009/07/06 23:57:26 eml Exp $

#ifndef GCP_UTIL_RUNNABLETESTCLASS_H
#define GCP_UTIL_RUNNABLETESTCLASS_H

/**
 * @file RunnableTestClass.h
 * 
 * Tagged: Sat Jan  1 23:52:43 CST 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:26 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Runnable.h"

namespace gcp {
  namespace util {
    
    class RunnableTestClass : public Runnable {
    public:
      
      /**
       * Constructor.
       */
      RunnableTestClass(bool spawn);
      
      /**
       * Destructor.
       */
      virtual ~RunnableTestClass();
      
      static RUN_FN(runFn);
      
      void run();
      
    }; // End class RunnableTestClass
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_RUNNABLETESTCLASS_H
