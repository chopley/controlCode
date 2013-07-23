// $Id: NedReader.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_NEDREADER_H
#define GCP_UTIL_NEDREADER_H

/**
 * @file NedReader.h
 * 
 * Tagged: Fri May 25 22:36:57 PDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author username: Command not found.
 */
#include "gcp/util/common/DecAngle.h"
#include "gcp/util/common/Declination.h"
#include "gcp/util/common/HourAngle.h"
#include "gcp/util/common/PtSrcReader.h"

#include <string>
#include <vector>
#include <list>
#include <sstream>

namespace gcp {
  namespace util {

    class NedReader : public PtSrcReader {
    public:

      enum {
	ANY,
	ALL,
	NOT,
	NON
      };

      enum {
	CLASSIFIED_EXTRAGALACTIC   = 0x1,
	UNCLASSIFIED_EXTRAGALACTIC = 0x2,
	GALAXY_COMPONENT           = 0x4,
	TYPE_ALL = 	CLASSIFIED_EXTRAGALACTIC | UNCLASSIFIED_EXTRAGALACTIC | GALAXY_COMPONENT
      };

      enum {

	OBJ_NONE            =      0x0,
	OBJ_ALL             =      0xFFFFFFFF,

	// Classified Extragalactic Objects

	GALAXY              =      0x1,
	GALAXY_PAIR         =      0x2,
	GALAXY_TRIPLET      =      0x4,
	GALAXY_GROUP        =      0x8,
	GALAXY_CLUSTER      =     0x10,
	QSO                 =     0x20,
	QSO_GROUP           =     0x40,
	GRAV_LENS           =     0x80,
	DAMPED_LYMAN_ALPHA  =    0x100,
	ABSORB_LINE_SYSTEM  =    0x200,
	EMISSION_LINE_SRC   =    0x400,

	// Unclassified extragalactic candidates

	RADIO_SRC           =    0x800,
	SUBMM_SRC           =   0x1000,
	IR_SRC              =   0x2000,
	VISUAL_SRC          =   0x4000,
	UV_EXCESS_SRC       =   0x8000,
	XRAY_SRC            =  0x10000,
	GAMMARAY_SRC        =  0x20000,

	// Components of Galaxies

	SUPERNOVA           =  0x40000,
	HII_REGION          =  0x80000,
	PLANETARY_NEBULA    = 0x100000,
	SUPERNOVA_REMNANT   = 0x200000,
      };

      struct Obj {
	unsigned type_;
	unsigned short id_;
	std::string inputName_;
	std::string outputName_;
	std::string expl_;

	Obj(unsigned type, unsigned short id, std::string inputName, std::string outputName, std::string expl) {
	  type_        = type;
	  id_          = id;
	  inputName_   = inputName;
	  outputName_  = outputName;
	  expl_        = expl;
	};

      };

      enum {
	CAT_ONE   = 0x1,
	CAT_TWO   = 0x2,
	CAT_THREE = 0x4,
	CAT_FOUR  = 0x8,
      };

      enum {
	CAT_NONE  =     0x0,
	CAT_ALL   =     0xFFFFFFFF,

	CAT_ABELL =     0x1,

	CAT_NGC   =     0x2,

	CAT_3C    =     0x4,
	CAT_4C    =     0x8,
	CAT_5C    =    0x10,
	CAT_6C    =    0x20,
	CAT_7C    =    0x40,
	CAT_8C    =    0x80,
	CAT_9C    =   0x100,
	
	CAT_87GB  =   0x200,
	CAT_XMM   =   0x400,
	CAT_WARP  =   0x800,
	CAT_WMAP  =  0x1000,

	CAT_MG    =  0x2000,
	CAT_MG1   =  0x4000,
	CAT_MG2   =  0x8000,
	CAT_MG4   = 0x10000,
	CAT_MG3   = 0x20000,

	CAT_MESSIER = 0x40000,
	CAT_MACS    = 0x80000, // Massive cluster survey (from the literatur)e

	CAT_SCUBA   =0x100000, // Massive cluster survey (from the literature)
      };

      // Operators for including/excluding catalogs

      struct Catalog {
	unsigned type_;
	unsigned short id_;
	std::string inputName_;
	std::string outputName_;
	std::string expl_;

	Catalog(unsigned type, unsigned short id, std::string inputName, std::string outputName, std::string expl) {
	  type_       = type;
	  id_         = id;
	  inputName_  = inputName;
	  outputName_ = outputName;
	  expl_ = expl;
	};

      };

      enum {
	ERROR_ARGS      = 1,
	ERROR_CURL_INIT = 2
      };

      enum {
	OPTION_FALSE = 0,
	OPTION_TRUE  = 1
      };

      enum {
	FLAG_DEFAULT = 0 
      };

      //------------------------------------------------------------
      // Methods for parsing the response from the NED database server
      //------------------------------------------------------------

      void initializeResponseParse();

      // Return true when we are at the end of the response

      bool atEndOfResponse();
      
      // Parse the next source entry

      PtSrcReader::Source readNextSourceFromResponse();

      //------------------------------------------------------------      
      // Define stubs
      //------------------------------------------------------------      

      void openCatalogFile();
      void closeCatalogFile();

      // Catalog-specific function to read the next entry from a file

      PtSrcReader::Source readNextEntry();

      // Return true if we are at the end of the catalog file

      bool eof();

      // Apply any corrections to convert the catalog values

      void applyCorrections(PtSrcReader::Source& src);

      // Calculate the minimal RA range we need to search

      void setRaRange(HourAngle& ra, Declination& dec, Angle& radius);

      //------------------------------------------------------------      
      // Specific members
      //------------------------------------------------------------      

      std::string::size_type start_;
      std::string::size_type stop_;

      static std::vector<Obj>     objects_;
      static std::vector<Catalog> catalogs_;

      // A stream containing the last read information returned from
      // the URL fetch

      std::ostringstream lastRead_;

      std::list<PtSrcReader::Source> srcs_;
      std::list<PtSrcReader::Source>::iterator srcIter_;
      
      /**
       * Constructor.
       */
      NedReader();

      /**
       * Destructor.
       */
      virtual ~NedReader();

      void getUrl(std::string url);

      unsigned short objIdToNedId(unsigned short id);
      unsigned short catIdToNedId(unsigned short id);

      void
	constructNearPositionSearchUrl(std::ostringstream& os, 
				       HourAngle ra, Declination dec,
				       Angle radius, 
				       unsigned objIncMask, unsigned objExcMask, 
				       unsigned catMask,    unsigned objSel,     unsigned catSel);
      
      // Add an object to a mask

      void maskObject(std::string name, unsigned& mask);

      void includeObjects(std::ostringstream& os, unsigned mask);
      void excludeObjects(std::ostringstream& os, unsigned mask);

      void addToObjectIncludeMask(unsigned mask);
      void addToObjectExcludeMask(unsigned mask);

      void setObjectIncludeMask(unsigned mask);
      void setObjectExcludeMask(unsigned mask);

      void clearObjectIncludeMask();
      void clearObjectExcludeMask();

      void matchAny(std::ostringstream& os);
      void matchAll(std::ostringstream& os);

      // Add an catalog to a mask

      void maskCatalog(std::string name, unsigned& mask);

      void selectCatalogs(std::ostringstream& os, unsigned mask);

      void addToCatalogMask(unsigned mask);
      void setCatalogMask(unsigned mask);
      void clearCatalogMask();

      void setObjectSelection(unsigned type);
      void setCatalogSelection(unsigned type);

      // Initializer for static array

      static std::vector<Obj>     initObjects();
      static std::vector<Catalog> initCatalogs();

      // A function that will be called to handle data from a call to
      // perform

      static size_t handle_data(void* buffer, size_t size, size_t nmemb, void* userp);

      PtSrcReader::Source parseSource(std::string line);

      void removeDuplicates();

      void listSources();

      std::vector<std::string> listObjects(unsigned short id);
      std::vector<std::string> listCatalogs();

      unsigned objIncMask_;
      unsigned objExcMask_;
      unsigned catMask_;

      unsigned objSel_;
      unsigned catSel_;

    }; // End class NedReader

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_NEDREADER_H
