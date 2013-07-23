#include "gcp/util/common/Exception.h"
#include "gcp/util/common/FirstTxtReader.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/String.h"

#include <iomanip>
#include <string.h>

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
FirstTxtReader::FirstTxtReader(std::string catalogFile):
  PtSrcReader(catalogFile), FirstReader() 
{
  initialize();
}

/**.......................................................................
 * Constructor.
 */
FirstTxtReader::FirstTxtReader() : PtSrcReader(), FirstReader() 
{
  initialize();
}

/**.......................................................................
 * Initialize critical members
 */
void FirstTxtReader::initialize() 
{
  status_   = 0;
  fitsFile_ = 0;
}

/**.......................................................................
 * Destructor.
 */
FirstTxtReader::~FirstTxtReader() {}

/**.......................................................................
 * Open the catalog file
 */
void FirstTxtReader::openCatalogFile()
{
  closeCatalogFile();

  ifStr_.open(catalogFile_.c_str());
  ifStr_.clear();

  if(!ifStr_.is_open())
    ThrowError("Couldn't open file: " << catalogFile_);
}

/**.......................................................................
 * Close the catalog file
 */
void FirstTxtReader::closeCatalogFile()
{
  if(ifStr_.is_open()) {
    ifStr_.close();
  }
}

/**.......................................................................
 * Read the next entry from the catalog file
 */
PtSrcReader::Source FirstTxtReader::readNextEntry()
{
  static String line;
  static PtSrcReader::Source src;

  if(!ifStr_.is_open())
    openCatalogFile();

  if(!ifStr_.eof()) {
    line.initialize();
    getline(ifStr_, line.str());

    if(line.str().size() > 0)
      src = convertToSource(line);
  }

  return src;
}

/**.......................................................................
 * After a line has been read, this method should be called to convert
 * the data to a Source structure.
 */
PtSrcReader::Source FirstTxtReader::convertToSource(String& str)
{
  static PtSrcReader::Source src;
  static double hr,min,sec;
  static double deg,amin,asec;

  char* tok=NULL;

  hr  = strtod(strtok((char*)str.str().c_str(), " "), NULL);
  min = strtod(strtok(NULL, " "), NULL);
  sec = strtod(strtok(NULL, " "), NULL);

  src.ra_.setHours(hr, min, sec);

  deg  = strtod(strtok(NULL, " "), NULL);
  amin = strtod(strtok(NULL, " "), NULL);
  asec = strtod(strtok(NULL, " "), NULL);

  src.dec_.setDegrees(deg, amin, asec);

  tok = strtok(NULL, " ");

  if(strcmp(tok, "W")==0) {
    src.warn_ = true;
    src.peak_.setMilliJy(strtod(strtok(NULL, " "), NULL));
  } else {
    src.warn_ = false;
    src.peak_.setMilliJy(strtod(tok, NULL));
  }

  src.int_.setMilliJy(strtod(strtok(NULL, " "), NULL));
  src.rms_.setMilliJy(strtod(strtok(NULL, " "), NULL));

  src.decMaj_.setArcSec(strtod(strtok(NULL, " "), NULL));
  src.decMin_.setArcSec(strtod(strtok(NULL, " "), NULL));
  src.decPa_.setDegrees(strtod(strtok(NULL, " "), NULL));

  src.fitMaj_.setArcSec(strtod(strtok(NULL, " "), NULL));
  src.fitMin_.setArcSec(strtod(strtok(NULL, " "), NULL));
  src.fitPa_.setDegrees(strtod(strtok(NULL, " "), NULL));

  src.name_ = strtok(NULL, " ");

  return src;
}

/**.......................................................................
 * Return true if the input stream for the catalog file is at the EOF
 * mark
 */
bool FirstTxtReader::eof()
{
  return ifStr_.eof();
}

/**.......................................................................
 * Method to convert the FIRST ASCII-style catalog to an NVSS-style
 * FITS BINTABLE extension
 */ 
void FirstTxtReader::convertCatalog()
{
  // Construct the list of RA range indices

  indexSources();

  // Now create the FITS file into which we will write the data

  createFitsCatalogFile();

  // Create the FITS binary table

  createFitsBinTable(nSrc_);

  // Make sure the catalog file is open

  openCatalogFile();
  
  // Iterate, reading from the catalog file until the EOF is reached
  
  long iRow = 0;
  PtSrcReader::Source src;
  while(!eof()) {
    src = readNextEntry();
    writeFitsEntry(src, ++iRow);
  }

  // Close the catalog file

  closeCatalogFile();

  // And close the FITS file 

  if(fits_close_file(fitsFile_, &status_))
    throwCfitsioError(status_);
}

/**.......................................................................
 * Create a dummy image file to which the catalog will be appended
 */
void FirstTxtReader::createFitsCatalogFile()
{
  // Create the FITS file

  if(fits_create_file(&fitsFile_, "first.fits", &status_))
    throwCfitsioError(status_);
  
  long naxes[2] = {5,5};
  unsigned short array[5][5];
  
  // Create the image header

  if(fits_create_img(fitsFile_, USHORT_IMG, 2, naxes, &status_))
    throwCfitsioError(status_);
  
  // Write the array of unsigned integers to the FITS file
  
  if(fits_write_img(fitsFile_, TUSHORT, 1, 25, array[0], &status_))
    throwCfitsioError(status_);
  
  // And close the file

  if(fits_close_file(fitsFile_, &status_))
    throwCfitsioError(status_);
}

/**.......................................................................
 * Write a single source as a FITS binary table entry
 */
void FirstTxtReader::writeFitsEntry(PtSrcReader::Source& src, long iRow)
{
  long firstEl  = 1;
  long nRow     = 1;

  double dVal = src.ra_.degrees();
  fits_write_col(fitsFile_, TDOUBLE,  1, iRow, firstEl, 1, &dVal,
		 &status_);

  dVal = src.dec_.degrees();
  fits_write_col(fitsFile_, TDOUBLE,  2, iRow, firstEl, 1, &dVal,
		 &status_);
  
  float fVal = (float)src.warn_;
  fits_write_col(fitsFile_, TFLOAT,   3, iRow, firstEl, 1, &fVal,
		 &status_);

  fVal = src.peak_.Jy();
  fits_write_col(fitsFile_, TFLOAT,   4, iRow, firstEl, 1, &fVal,
		 &status_);

  fVal = src.int_.Jy();
  fits_write_col(fitsFile_, TFLOAT,   5, iRow, firstEl, 1, &fVal,
		 &status_);

  fVal = src.rms_.Jy();
  fits_write_col(fitsFile_, TFLOAT,   6, iRow, firstEl, 1, &fVal,
		 &status_);

  fVal = src.decMaj_.degrees();
  fits_write_col(fitsFile_, TFLOAT,   7, iRow, firstEl, 1, &fVal,
		 &status_);

  fVal = src.decMin_.degrees();
  fits_write_col(fitsFile_, TFLOAT,   8, iRow, firstEl, 1, &fVal,
		 &status_);

  fVal = src.decPa_.degrees();
  fits_write_col(fitsFile_, TFLOAT,   9, iRow, firstEl, 1, &fVal,
		 &status_);

  fVal = src.fitMaj_.degrees();
  fits_write_col(fitsFile_, TFLOAT,  10, iRow, firstEl, 1, &fVal,
		 &status_);

  fVal = src.fitMin_.degrees();
  fits_write_col(fitsFile_, TFLOAT,  11, iRow, firstEl, 1, &fVal,
		 &status_);

  fVal = src.fitPa_.degrees();
  fits_write_col(fitsFile_, TFLOAT,  12, iRow, firstEl, 1, &fVal,
		 &status_);
}

/**.......................................................................
 * Append a FITS binary table header
 */
void FirstTxtReader::createFitsBinTable(long nRow)
{
  int nCol = 12;                  // FIRST catalog has 13 columns
  char filename[] = "first.fits"; // Name for new FITS file
  char extname[]  = "AIPS VL";    // extension name 
  
  // Define the name, datatype, and physical units for the columns
  
  char *ttype[] = { "RA(2000)",  "DEC(2000)", "WARN",       "PEAK INT",   
		    "INT FLUX",  "I RMS",     "DEC MAJ AX", "DEC MIN AX", 
		    "DEC PA",    "FIT MAJ AX","FIT MIN AX", "FIT PA"};    

  
  char *tform[] = { "1D",        "1D",        "1E",         "1E",
		    "1E",        "1E",        "1E",         "1E",
		    "1E",        "1E",        "1E",         "1E"};
  
  char *tunit[] = { "DEGREE",   "DEGREE",    "T/F",         "JY/BEAM" , 
		    "JY",       "JY/BEAM",   "DEGREE",      "DEGREE",
		    "DEGREE",   "DEGREE",    "DEGREE",      "DEGREE"};

  // Open the named file

  if(fits_open_file(&fitsFile_, "first.fits", READWRITE, &status_)) 
    throwCfitsioError(status_);

  // Now move to end of 1st HDU

  int hduType;
  if(fits_movabs_hdu(fitsFile_, 1, &hduType, &status_)) 
    throwCfitsioError(status_);

  // Append a new empty binary table onto the FITS file
  
  if(fits_create_tbl(fitsFile_, BINARY_TBL, nRow, nCol, ttype, tform,
		     tunit, extname, &status_))
    throwCfitsioError(status_);

  // Add keywords for indexing the file

  char keyName[8], expl[8];
  for(unsigned iRa=0; iRa < 24; iRa++) {
    sprintf(keyName, "INDEX%02d", iRa);
    sprintf(expl,    "RA %02d", iRa);
    if(fits_update_key(fitsFile_, TLONG, keyName, &sourceIndices_[iRa],
		       expl, &status_))
      throwCfitsioError(status_);
  }
};



