// $Id: ArchiverWriterFrame.h,v 1.1.1.1 2009/07/06 23:57:24 eml Exp $

#ifndef GCP_CONTROL_ARCHIVERWRITERFRAME_H
#define GCP_CONTROL_ARCHIVERWRITERFRAME_H

/**
 * @file ArchiverWriterFrame.h
 * 
 * Tagged: Tue Nov 14 07:32:14 PST 2006
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:24 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/ArchiverWriter.h"
#include "gcp/util/common/ArrayDataFrameManager.h"

#include "gcp/control/code/unix/libunix_src/common/netbuf.h"
#include "gcp/control/code/unix/libunix_src/common/regdata.h"
#include "gcp/control/code/unix/libunix_src/common/genericregs.h"

#include <stdio.h>

namespace gcp {
  namespace util {

    class ArchiverWriterFrame : public ArchiverWriter {
    public:

      /**
       * Constructor.
       */
      ArchiverWriterFrame();

      /**
       * Destructor.
       */
      virtual ~ArchiverWriterFrame();

      int chdir(char* dir);
      int openArcfile(char* dir);
      int openArcfile(std::string dir);
      void closeArcfile();
      void flushArcfile();
      int writeIntegration();
      int saveIntegration();

      void setFileSize(unsigned fileSize);

      ArrayDataFrameManager* frame() {
	return frame_->fm;
      }

    private:
      
      gcp::control::NetBuf*     net_;
      char*       dir_;
      char*       path_;
      int         nrecorded_;
      int         fileSize_;
      FILE*       fp_;
      ArrayMap*   arrayMap_;
      RegRawData* frame_;

    }; // End class ArchiverWriterFrame

  } // End namespace control
} // End namespace gcp



#endif // End #ifndef GCP_CONTROL_ARCHIVERWRITERFRAME_H
