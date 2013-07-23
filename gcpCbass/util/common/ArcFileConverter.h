#ifndef GCP_UTIL_ARCFILECONVERTER_H
#define GCP_UTIL_ARCFILECONVERTER_H

/**
 * @file ArcFileConverter.h
 * 
 * Tagged: Wed 08-Feb-06 13:46:21
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/ArchiverWriterDirfile.h"
#include "gcp/control/code/unix/libmonitor_src/monitor_stream.h"

namespace gcp {
  namespace util {
    
    class ArcFileConverter {
    public:
      
      /**
       * Constructor.
       */
      ArcFileConverter(std::string datfileName,
		       std::string dirfileName);
      
      /**
       * Destructor.
       */
      virtual ~ArcFileConverter();
      
      void convert();

    private:

      gcp::control::MonitorStream* ms_;

      gcp::util::ArchiverWriterDirfile writer_;

      gcp::control::MsReadState readNextFrame();

    }; // End class ArcFileConverter
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_ARCFILECONVERTER_H
