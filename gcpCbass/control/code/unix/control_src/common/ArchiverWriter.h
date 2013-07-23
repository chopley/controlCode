#ifndef GCP_CONTROL_ARCHIVERWRITER_H
#define GCP_CONTROL_ARCHIVERWRITER_H

/**
 * @file ArchiverWriter.h
 * 
 * Tagged: Fri 27-Jan-06 14:31:33
 * 
 * @author Erik Leitch
 */
class Archiver;

namespace gcp {
  namespace control {

    class ArchiverWriter {
    public:
      
      /**
       * Constructor.
       */
      ArchiverWriter() {};
      ArchiverWriter(Archiver* arc) : arc_(arc) {};
      
      /**
       * Destructor.
       */
      virtual ~ArchiverWriter() {};
      
      virtual int openArcfile(char* dir) {};
      virtual void closeArcfile() {};
      virtual void flushArcfile() {};
      virtual int writeIntegration() {};
      virtual bool isOpen() {};

    protected:

      Archiver* arc_;

    }; // End class ArchiverWriter
    
  } // End namespace control
} // End namespace gcp


#endif // End #ifndef GCP_CONTROL_ARCHIVERWRITER_H
