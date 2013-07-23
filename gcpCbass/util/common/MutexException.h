#ifndef GCP_UTIL_MUTEXEXCEPTION_H
#define GCP_UTIL_MUTEXEXCEPTION_H

/**
 * @file MutexException.h
 * 
 * Tagged: Sat May 22 07:36:14 PDT 2004
 * 
 * @author Erik Leitch
 */
namespace gcp {
  namespace util {
    
    class MutexException {
    public:
      
      /**
       * Constructor.
       */
      MutexException();
      
      /**
       * Destructor.
       */
      virtual ~MutexException();
      
    private:
    }; // End class MutexException
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_MUTEXEXCEPTION_H
