#ifndef GCP_CONTROL_ARCHIVERWRITERFRAME_H
#define GCP_CONTROL_ARCHIVERWRITERFRAME_H

/**
 * @file ArchiverWriterFrame.h
 * 
 * Tagged: Fri 27-Jan-06 14:35:19
 * 
 * @author Erik Leitch
 */
#include "gcp/control/code/unix/control_src/common/ArchiverWriter.h"

namespace gcp {
  namespace control {
    
    class ArchiverWriterFrame : public ArchiverWriter {
    public:
      
      /**
       * Constructor.
       */
      ArchiverWriterFrame(Archiver* parent);
      
      /**
       * Copy Constructor.
       */
      ArchiverWriterFrame(const ArchiverWriterFrame& objToBeCopied);
      
      /**
       * Copy Constructor.
       */
      ArchiverWriterFrame(ArchiverWriterFrame& objToBeCopied);
      
      /**
       * Const Assignment Operator.
       */
      void operator=(const ArchiverWriterFrame& objToBeAssigned);
      
      /**
       * Assignment Operator.
       */
      void operator=(ArchiverWriterFrame& objToBeAssigned);
      
      /**
       * Destructor.
       */
      virtual ~ArchiverWriterFrame();
      
      int chdir(char* dir);
      int openArcfile(char* dir);
      void closeArcfile();
      void flushArcfile();
      int writeIntegration();
      bool isOpen();

    }; // End class ArchiverWriterFrame
    
  } // End namespace control
} // End namespace gcp


#endif // End #ifndef GCP_CONTROL_ARCHIVERWRITERFRAME_H
