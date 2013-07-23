// $Id: FirstFitsReader.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_FIRSTFITSREADER_H
#define GCP_UTIL_FIRSTFITSREADER_H

/**
 * @file FirstFitsReader.h
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
#include "gcp/util/common/PtSrcFitsReader.h"
#include "gcp/util/common/String.h"

namespace gcp {
  namespace util {

    class FirstFitsReader : public PtSrcFitsReader {
    public:

      /**
       * Constructor.
       */
      FirstFitsReader(std::string catalogFile);
      FirstFitsReader();

      /**
       * Destructor.
       */
      virtual ~FirstFitsReader();

      //------------------------------------------------------------
      // Overloaded base-class methods from PtSrcFitsReader

      PtSrcReader::Source parseData();

      void readFitsData(long startRow, long nElement);

      void applyCorrections(PtSrcReader::Source& src);

    private:

      FirstReader firstReader_;
      float warns_[PtSrcFitsReader::chunkSize_];
      float intFluxes_[PtSrcFitsReader::chunkSize_];
      float decMajorAxes_[PtSrcFitsReader::chunkSize_];
      float decMinorAxes_[PtSrcFitsReader::chunkSize_];
      float decPositionAngles_[PtSrcFitsReader::chunkSize_];

    }; // End class FirstFitsReader

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_FIRSTFITSREADER_H
