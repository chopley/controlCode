// $Id: FirstTxtReader.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_FIRSTTXTREADER_H
#define GCP_UTIL_FIRSTTXTREADER_H

/**
 * @file FirstTxtReader.h
 * 
 * Tagged: Tue Aug  8 18:13:20 PDT 2006
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author Erik Leitch
 */
#include <fstream>
#include <string>
#include <vector>

#include "gcp/util/common/Angle.h"
#include "gcp/util/common/DecAngle.h"
#include "gcp/util/common/FirstReader.h"
#include "gcp/util/common/Flux.h"
#include "gcp/util/common/HourAngle.h"
#include "gcp/util/common/PtSrcReader.h"
#include "gcp/util/common/String.h"

#include "gcp/cfitsio/common/fitsio.h"

namespace gcp {
  namespace util {

    class FirstTxtReader : public PtSrcReader, public FirstReader {
    public:

      /**
       * Constructor.
       */
      FirstTxtReader(std::string catalogFile);
      FirstTxtReader();

      void initialize(); 

      /**
       * Destructor.
       */
      virtual ~FirstTxtReader();

      // The catalog file handle

      std::ifstream ifStr_;

      // Open/close the catalog file

      void openCatalogFile();
      void closeCatalogFile();

      Source convertToSource(gcp::util::String& str);

      // Check if we are at the end of the file

      bool eof();

      // Read the next entry from the catalog file

      PtSrcReader::Source readNextEntry();

      // Method to convert the FIRST ASCII-style catalog to an
      // NVSS-style FITS BINTABLE extension

      void convertCatalog();

      // Create a dummy image file to which the catalog will be
      // appended

      void createFitsCatalogFile();

      // Write a single source as a FITS binary table entry

      void writeFitsEntry(PtSrcReader::Source& src, long iRow);

      // Append a FITS binary table header

      void createFitsBinTable(long nRow);

    private:

      fitsfile* fitsFile_;
      int status_;

    }; // End class FirstTxtReader

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_FIRSTTXTREADER_H
