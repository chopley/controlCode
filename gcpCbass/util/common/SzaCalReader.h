#ifndef GCP_UTIL_GCPCALREADER_H
#define GCP_UTIL_GCPCALREADER_H

/**
 * @file NvssReader.h
 * 
 * Tagged: Tue Aug 15 18:00:57 PDT 2006
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:27 $
 * 
 * @author username: Command not found.
 */
#include "gcp/util/common/PtSrcReader.h"
#include "gcp/util/common/String.h"

#include <fstream>

namespace gcp {
  namespace util {

    class SzaCalReader : public PtSrcReader {
    public:

      /**
       * Constructor.
       */
      SzaCalReader(std::string catalogFile);
      SzaCalReader();

      /**
       * Destructor.
       */
      virtual ~SzaCalReader();

    public:

      // Catalog file handling

      void openCatalogFile();
      void closeCatalogFile();

      // Catalog-specific function to read the next entry from a file

      PtSrcReader::Source readNextEntry();

      // Return true if we are at the end of the catalog file

      bool eof();

      // After a line has been read, this method should be called to
      // convert the data to a Source structure.

      PtSrcReader::Source convertToSource(String& str);

    private:

      // The catalog file handle

      std::ifstream ifStr_;


    }; // End class SzaCalReader

  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_GCPCALREADER_H
