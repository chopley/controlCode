#ifndef GCP_UTIL_FITSIO_H
#define GCP_UTIL_FITSIO_H

/**
 * @file FitsIo.h
 * 
 * Tagged: Mon May  2 19:04:15 UTC 2005
 * 
 * @author 
 */
#include <string>

#include "gcp/util/common/DataType.h"
#include "gcp/util/common/DecAngle.h"
#include "gcp/util/common/HourAngle.h"
#include "gcp/util/common/Frequency.h"

// Functions of the following type are called by put_phdu() to
// convert a passed string value to the appropriate type.

#define WRITEFN(fn) void (fn)(char *buf, const char *val);

// Functions of the following type are called by read_phdu() to
// convert a passed string value to the appropriate type.

#define READFN(fn)  void (fn)(FitsIo::FitsDataType& data, char *str);

namespace gcp {
  namespace util {
    
    class FitsIo {
    public:
      
      /**
       * Constructor.
       */
      FitsIo();
      
      /**
       * Destructor.
       */
      virtual ~FitsIo();
      
      //------------------------------------------------------------
      // Some const members of this class
      //------------------------------------------------------------
      
      // Length of an ASCII header
      
      static const unsigned nBytePerHeader_=80;
      
      // Number of headers per data record
      
      static const unsigned nHeaderPerDataRecord_ = 36;
      
      // Length of a data record
      
      static const unsigned nBytePerDataRecord_ = 
	nBytePerHeader_ * nHeaderPerDataRecord_;
      
      // Length of the header keyword.
      
      static const unsigned nBytePerKeyword_=8;
      
      // A logical value 'T' or 'F' for logical keywords must occur in
      // this position in a string:
      
      static const unsigned logPos_=30;      
      
      // The length of a single record of the frequency table
      
      static const unsigned nBytePerFrequencyTableEntry_ = 32;
      
      // The number of seconds per day
      
      static const unsigned secondsPerDay_ = 86400;
      
      // Enumerate recognized Fits axis types.
      
      enum FitsAxis {
	AX_DEG,        /* Degrees -- type of RA/DEC */
	AX_RAD,
	AX_UV,
	AX_UNKNOWN
      };
      
      // And a container for associating a header string with this
      // unit.
      
      struct FitsAxisCard {
	char *str_;
	char *label_;
	FitsIo::FitsAxis axis_;
      };
      
      // A static list of recognized FITS axis types
      
      static FitsAxisCard fitsAxes_[];
      static unsigned nFitsAxes_;
      
      // Enumerate recognized Fits bunits.
      
      enum FitsBunit {
	BU_MJYSR,     /* MJy/sr */
	BU_JYBEAM,    /* Jy/Beam */
	BU_MUK,       /* MicroKelvin */
	BU_UNKNOWN
      };
      
      // And a container for associating a header string with this
      // bunit.
      
      struct FitsBunitCard {
	char *str_;
	char *label_;
	FitsBunit bunit_;
      };
      
      // A struct for handling all possible FITS data types
      
      struct FitsDataType {
	gcp::util::DataType stdVal_;
	std::string stringVal_;
	FitsIo::FitsBunit bunitVal_;
	FitsIo::FitsAxis axisVal_;
      };
      
      // A static list of recognized FITS axis types
      
      static FitsBunitCard fitsUnits_[];
      static unsigned nFitsUnits_;
      
      // Declare a container for a single header card.
      
      struct Phdu {
	char name_[FitsIo::nBytePerKeyword_+1];
	READFN(*readfn_);
	WRITEFN(*writefn_);
	int required_;
      };
      
      static FitsIo::Phdu phdus_[];
      static unsigned nPhdus_;
      
      // Write a header key to a file.
      
      void putPhdu(const char *name, const char *val, const char *comment, 
		   FILE *fp=0);
      
      // Methods for formatting various FITS keyword types
      
      static void nullStr(char *buf, const char *val);
      static void  logStr(char *buf, const char *val);
      static void  intStr(char *buf, const char *val);
      static void  fltStr(char *buf, const char *val);
      static void  strStr(char *buf, const char *val);
      
      // Methods for reading various FITS keyword types
      
      static void  rdNull(FitsDataType& data, char *str);
      static void   rdLog(FitsDataType& data, char *str);
      static void   rdStr(FitsDataType& data, char *str);
      static void   rdInt(FitsDataType& data, char *str);
      static void   rdFlt(FitsDataType& data, char *str);
      static void rdBunit(FitsDataType& data, char *str);
      static void  rdAxis(FitsDataType& data, char *str);
      
      //------------------------------------------------------------
      // Methods to set some information needed to write the FITS file
      //------------------------------------------------------------
      
      void setObsRa(HourAngle ra) {
	obsRa_ = ra;
      }
      
      void setObsDec(DecAngle dec) {
	obsDec_ = dec;
      }
      
      void setTelescopeName(std::string telescope) {
	telescope_ = telescope;
      }
      
      // Install a set of telescope locations
      
      void setTelescopeLocations(std::vector<std::vector<double> >& locations);
      
      void setInstrument(std::string instrument) {
	instrument_ = instrument;
      } 
      
      void setSourceName(std::string srcName) {
	srcName_ = srcName;
      }
      
      void setNumberOfIfs(unsigned nIf) {
	nIf_ = nIf;
	ifFrequencies_.resize(0);
      }
      
      // Install a set of IF frequencies
      
      void setIfFrequencies(std::vector<Frequency>& frequencies);
      
      void setDeltaIfFrequencies(std::vector<Frequency>& frequencies);
      
      // An alternate method for specifying IF frequencies is to use a
      // starting frequency and a delta
      
      void setStartingIfFrequency(Frequency frequency) {
	startingIfFrequency_ = frequency;
      }
      
      void setDeltaIfFrequency(Frequency frequency) {
	deltaIfFrequency_ = frequency;
	deltaIfFrequencies_.resize(0);
      }
      
      void setNumberOfChannelsPerIf(unsigned nChan) {
	nChannel_ = nChan;
      }
      
      void setDeltaChannelFrequency(Frequency frequency) {
	deltaChannelFrequency_ = frequency;
      }
      
      void setNumberOfFrames(unsigned nFrame) {
	nFrame_ = nFrame;
      }
      
      void setNumberOfBaselines(unsigned nBaseline) {
	nBaseline_ = nBaseline;
      }
      
      void setNumberOfStokesParameters(unsigned nStokes) {
	nStokes_ = nStokes;
      }
      
      void setDate();
      
      void checkParameters();
      
      //------------------------------------------------------------
      // Methods to write the file
      //------------------------------------------------------------
      
      void openFile(std::string fileName);
      
      //------------------------------------------------------------
      // Methods to write headers
      //------------------------------------------------------------
      
      // All headers will be initialized and finished the same way
      
      void initHeader();
      void finishHeader(FILE* fp=0);
      
      // Methods to write the bodies for different types of header
      
      void writeFileHeaderBody(FILE* fp=0);
      void writeAntennaTableHeaderBody(FILE* fp=0);
      void writeFrequencyTableHeaderBody(FILE* fp=0);
      
      // Methods to write the whole header, for different header types
      
      void writeFileHeader(FILE* fp=0);
      void writeAntennaTableHeader(FILE* fp=0);
      void writeFrequencyTableHeader(FILE* fp=0);
      
      //------------------------------------------------------------
      // Methods to write the data segment
      //------------------------------------------------------------
      
      void initVisibilityData();
      void writeVisibilityDataBody(double* vis, double* date, double* uvw, FILE* fp=0);
      void finishVisibilityData(FILE* fp=0);
      
      void writeVisibilityData(double* vis, double* date, double* uvw, FILE* fp=0);
      
      //------------------------------------------------------------
      // Methods to write the antenna table
      //------------------------------------------------------------
      
      void initAntennaTableDataBody();
      void writeAntennaTableDataBody(FILE* fp=0);
      void finishAntennaTableDataBody(FILE* fp=0);
      
      // Write the data for an antenna table
      
      void writeAntennaTableData(FILE* fp=0);
      void writeAntennaTableEntry(int i, double X, double Y, double Z, FILE* fp=0);
      
      // Write the whole antenna table
      
      void writeAntennaTable(FILE* fp=0);
      
      //------------------------------------------------------------
      // Methods to write the frequency table
      //------------------------------------------------------------
      
      void initFrequencyTableDataBody();
      void writeFrequencyTableDataBody(FILE* fp=0);
      void finishFrequencyTableDataBody(FILE* fp=0);
      
      void writeFrequencyTableData(FILE* fp=0);
      void writeFrequencyTable(FILE* fp=0);
      
      //-----------------------------------------------------------------------
      // Write a UVF file
      //-----------------------------------------------------------------------
      
      void writeUvfFile(double* data, double* date, double* uvw, FILE* fp=0);
      
      // A method to return whichever is the current file descriptor
      
      FILE* getFptr(FILE* fp);
      
    private:
      
      // Internal counters we will use when writing headers and data
      
      unsigned nHdu_;
      unsigned nDataByte_;
      
      //------------------------------------------------------------
      // Some information needed to write a FITS file
      //------------------------------------------------------------
      
      // The name of the telescope:
      
      std::string telescope_;
      std::string instrument_;
      std::string date_;
      
      // Information about the source
      
      HourAngle obsRa_;
      DecAngle obsDec_;
      std::string srcName_;
      
      // Information about the frequency
      
      // The number of IFs
      
      unsigned nIf_;
      Frequency startingIfFrequency_;
      Frequency deltaIfFrequency_;
      std::vector<Frequency> ifFrequencies_;
      std::vector<Frequency> deltaIfFrequencies_;
      
      // The number of channels per IF
      
      unsigned nChannel_;
      Frequency startingFrequency_;
      Frequency deltaChannelFrequency_;
      
      // The number of telescopes in our antenna table
      
      unsigned nTelescope_;      
      std::vector<std::vector<double> > locations_;
      
      // The number of frames
      
      unsigned nFrame_;
      
      // The number of baselines
      
      unsigned nBaseline_;
      
      // The number of Stokes parameters
      
      unsigned nStokes_;
      
      FILE* fp_;
      
      // Utility functions for converting from one Endianness to the
      // other
      
      void cp4r4(unsigned char *dest, unsigned char *orig, size_t nitem);
      
      // Orig: 8-byte datatype.  Dest: 8-byte datatype with byte
      // order reversed.
      
      void cp8r8(unsigned char *dest, unsigned char *orig, size_t nitem);
      
      void fwrite(unsigned int* ptr, size_t nel, FILE* stream);
      void fwrite(float* ptr,        size_t nel, FILE* stream);
      void fwrite(double* ptr,       size_t nel, FILE* stream);
      
    public:
      
      // Return the antenna indices associated with a given visibility
      // index.
      
      void getTelescopeIndices(unsigned baslineIndex, 
			       unsigned* iRow, unsigned* iCol, 
			       unsigned nTel);
      
    }; // End class FitsIo
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_FITSIO_H
