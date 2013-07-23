#include "gcp/util/common/SzaCalReader.h"
#include "gcp/util/common/Exception.h"

#include <stdlib.h>
#include <string.h>

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
SzaCalReader::SzaCalReader(std::string catalogFile):
  PtSrcReader(catalogFile) {}

/**.......................................................................
 * Constructor.
 */
SzaCalReader::SzaCalReader() : PtSrcReader() {}

/**.......................................................................
 * Destructor.
 */
SzaCalReader::~SzaCalReader() {}

/**.......................................................................
 * Open the catalog file
 */
void SzaCalReader::openCatalogFile()
{
  ifStr_.open(catalogFile_.c_str());
  ifStr_.clear();
  
  if(!ifStr_.is_open())
    ThrowSimpleError("Couldn't open file: " << catalogFile_);
}

/**.......................................................................
 * Close the catalog file
 */
void SzaCalReader::closeCatalogFile()
{
  if(ifStr_.is_open()) {
    ifStr_.close();
  }
}

/**.......................................................................
 * Read the next entry from the catalog file
 */
PtSrcReader::Source SzaCalReader::readNextEntry()
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
PtSrcReader::Source SzaCalReader::convertToSource(String& str)
{
  static PtSrcReader::Source src;
  static double hr,min,sec;
  static double deg,amin,asec;

  char* tok=NULL;

  src.name_ = strtok((char*)str.str().c_str(), " ");
  src.ra_.setHours(strtok(NULL, " "));
  src.dec_.setDegrees(strtok(NULL, " "));

  src.peak_.setJy(strtod(strtok(NULL, " "), NULL));
  src.specInd_ = strtod(strtok(NULL, " "), NULL);

  return src;
}

/**.......................................................................
 * Return true if the input stream for the catalog file is at the EOF
 * mark
 */
bool SzaCalReader::eof()
{
  return ifStr_.eof();
}
