// $Id: PtSrcReader.h,v 1.1.1.1 2009/07/06 23:57:26 eml Exp $

#ifndef GCP_UTIL_PTSRCREADER_H
#define GCP_UTIL_PTSRCREADER_H

/**
 * @file PtSrcReader.h
 * 
 * Tagged: Tue Aug 15 17:53:08 PDT 2006
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:26 $
 * 
 * @author username: Command not found.
 */
#include "gcp/util/common/Angle.h"
#include "gcp/util/common/Declination.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/Flux.h"
#include "gcp/util/common/HourAngle.h"
#include "gcp/util/common/String.h"

#include "gcp/cfitsio/common/fitsio.h"

#include <vector>

namespace gcp {
  namespace util {

    class PtSrcReader {
    public:

      /**
       * Structure to encapsulate information about a source
       */
      struct Source {

	// The name of the source

	String name_;

	// RA of the source, and estimated error

	HourAngle ra_;
	HourAngle raErr_;

	// DEC of the source, and estimated error

	Declination dec_;
	Declination decErr_;

	// The distance of this source from a field center

	Angle distance_;

	// Specific to FIRST -- will be set to 1 (true) if the source
	// may be a sidelobe of a bright nearby source

	bool warn_;

	// Peak flux and error in the flux (technically this is an
	// intensity, in flux/beam, and not a flux)

	Flux rawPeak_;

	Flux peak_;
	Flux peakErr_;

	// Rms on the peak flux

	Flux rms_;

	// Integrated flux and error

	Flux int_;
	Flux intErr_;

	//------------------------------------------------------------
	// Fitted parameters

	// Fitted major axis (before deconvolution with restoring
	// beam)

	Angle fitMaj_;
	Angle fitMajErr_;

	// Fitted minor axis (before deconvolution with restoring
	// beam)

	Angle fitMin_;
	Angle fitMinErr_;

	// Fitted position angle (before deconvolution with restoring
	// beam)

	Angle fitPa_;
	Angle fitPaErr_;

	//------------------------------------------------------------
	// Deconvolved parameters

	// Deconvolved major axis

	Angle decMaj_;
	Angle decMajErr_;

	// Deconvolved minor axis

	Angle decMin_;
	Angle decMinErr_;

	// Deconvolved position angle

	Angle decPa_;
	Angle decPaErr_;

	//------------------------------------------------------------
	// Restoring beam parameters for this source

	Angle resMaj_;
	Angle resMin_;
	Angle resPa_;

	//------------------------------------------------------------
	// For some sources, this will record a spectral index

	double specInd_;

	//------------------------------------------------------------
	// For some sources, we will record a survey name

	std::string survey_;

	static bool isLessThan(PtSrcReader::Source& src1, PtSrcReader::Source& src2) 
	{
	  return (src1.survey_ < src2.survey_) || (src1.survey_ == src2.survey_ && src1.name_.str() <= src2.name_.str());
	}

	static bool isEqualTo(PtSrcReader::Source& src1, PtSrcReader::Source& src2) 
	{
	  return (src1.survey_ == src2.survey_ && src1.name_.str() == src2.name_.str());
	}

      };

      // Constructor.

      PtSrcReader(std::string catalogFile);
      PtSrcReader();
      void initialize();

      // Destructor.

      virtual ~PtSrcReader();

      void setCatalogFile(std::string catalogFile);

      // Find sources within radius of the requested position

      std::vector<PtSrcReader::Source> findSources(HourAngle ra, Declination dec, Angle radius, 
						   Flux fMin=minFlux_, Flux fMax=maxFlux_, bool doPrint=true);

      // Return the number of sources

      unsigned countSources(HourAngle ra, Declination dec, Angle radius, 
			    Flux fMin=minFlux_, Flux fMax=maxFlux_);


      // Index the list of sources
      
      void indexSources();

      // Check if a position if within a given radius of the passed ra and dec

      bool checkAngle(PtSrcReader::Source& src, HourAngle& ra, Declination& dec, Angle& radius);

      // Friend funtion to print out source information

      friend std::ostream& gcp::util::operator<<(std::ostream& os, PtSrcReader::Source& src);
      void printHeader(std::ostream& os);

      // Catalog-specific functions to open/cose the catalog file

      virtual void openCatalogFile()  = 0;
      virtual void closeCatalogFile() = 0;

      // Catalog-specific function to read the next entry from a file

      virtual Source readNextEntry() = 0;

      // Return true if we are at the end of the catalog file

      virtual bool eof() = 0;

      // Apply any corrections to convert the catalog values

      virtual void applyCorrections(Source& src);

      // Calculate the minimal RA range we need to search

      virtual void setRaRange(HourAngle& ra, Declination& dec, Angle& radius);

      // Defaults for flux searching when none are specified

      static Flux minFlux_;
      static Flux maxFlux_;

      HourAngle raMin_;
      HourAngle raMax_;

    protected: 

      // The name of the catalog File

      std::string catalogFile_;

      //
      long sourceIndices_[25];

      // The number of sources in a catalog

      unsigned nSrc_;

      // Report an error generated by the cfitsio library
      
      void throwCfitsioError(int status);

    }; // End class PtSrcReader

    // A predicate for testing if a src equals another

    class Src_eq : public std::unary_function<PtSrcReader::Source, bool> {
      PtSrcReader::Source* src_;
    public:
      explicit Src_eq(PtSrcReader::Source* src) : src_(src) {}
      bool operator() (const PtSrcReader::Source* src) const {return (src_ != src && src_->survey_ == src->survey_ && src_->name_ == src->name_);}
    };

    // A predicate for testing if a src is lexically less than another

    struct Src_lt : public std::binary_function<PtSrcReader::Source, PtSrcReader::Source, bool> {
      bool operator() (PtSrcReader::Source& src1, PtSrcReader::Source& src2) const 
      {
	COUT("Here: Src_lt");
	return (src1.survey_ < src2.survey_ && src1.name_.str() < src2.name_.str());
      }
    };


  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_PTSRCREADER_H
