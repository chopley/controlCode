#ifndef GCP_UTIL_ARCHIVERWRITER_H
#define GCP_UTIL_ARCHIVERWRITER_H

/**
 * @file ArchiverWriter.h
 * 
 * Tagged: Fri 27-Jan-06 14:31:33
 * 
 * @author Erik Leitch
 */
class Archiver;

namespace gcp {
  namespace util {

    class ArchiverWriter {
    public:
      
      /**
       * Constructor.
       */
      ArchiverWriter() {};
      
      /**
       * Destructor.
       */
      virtual ~ArchiverWriter() {};
      
      virtual void closeArcfile() {};
      virtual void flushArcfile() {};
      virtual int writeIntegration() {};
      virtual bool isOpen() {};

    }; // End class ArchiverWriter
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_ARCHIVERWRITER_H
