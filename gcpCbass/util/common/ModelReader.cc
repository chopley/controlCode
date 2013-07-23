#include "gcp/control/code/unix/libunix_src/common/input.h"

#include "gcp/util/common/ModelReader.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

#include <iomanip>
#include <cmath>

using namespace std;

using namespace gcp::util;

const double ModelReader::arcSecPerRad_ = 206265;

/**.......................................................................
 * Constructors
 */
ModelReader::ModelReader() 
{
  raInterp_   = 0;
  decInterp_  = 0;
  distInterp_ = 0;

  raInterp_   = new QuadraticInterpolatorNormal(0.0);
  decInterp_  = new QuadraticInterpolatorNormal(0.0);
  distInterp_ = new QuadraticInterpolatorNormal(0.0);

}

ModelReader::ModelReader(std::string dir, std::string fileName) 
{
  raInterp_   = 0;
  decInterp_  = 0;
  distInterp_ = 0;

  raInterp_   = new QuadraticInterpolatorNormal(0.0);
  decInterp_  = new QuadraticInterpolatorNormal(0.0);
  distInterp_ = new QuadraticInterpolatorNormal(0.0);

  readFile(dir, fileName);
}

/**.......................................................................
 * Destructor
 */
ModelReader::~ModelReader() {}

void ModelReader::readFile(std::string dir, std::string fileName) 
{
  InputStream* stream=0;
  
  stream = new_InputStream();
  
  if(stream==0) 
    ThrowError("Unable to allocate a new input stream");
  
  try {
    
    if(open_FileInputStream(stream, (char*) dir.c_str(),
			    (char*)fileName.c_str())) 
      
      ThrowError("Unable to connect input stream to file: " 
		 << dir.c_str() << fileName.c_str());
    
    // Locate the first entry.
    
    if(input_skip_white(stream, 0, 0)) 
      ThrowError("Error");
    
    // Read to the end of the stream or error.
    
    while(stream->nextc != EOF) 
      readRecord(stream);
    
  } catch(const Exception& err) {
    
    // Close the file.
    
    if(stream != 0)
      del_InputStream(stream);
    
    throw err;
  }
  
  // Close the file.
  
  if(stream != 0)
    del_InputStream(stream);

  COUT("Read: " << mjd_.size() << " records"
       << "Starting Mjd: " << std::setprecision(12) << mjd_[0]
       << "Starting Mjd: " << std::setprecision(12) << mjd_[1]
       << " Ending Mjd: " << std::setprecision(12) << mjd_[mjd_.size()-1]);
}

void ModelReader::readRecord(InputStream* stream) 
{

  // Read the MJD
  
  readItem(stream);
  mjd_.push_back(atof(stream->work));
  
  // Read the Right-Ascension  (and skip it)
 
  readItem(stream);
  //  ra_.push_back(atof(stream->work));
  
  // Read the Declination (and skip it)
  
  readItem(stream); 
  //  dec_.push_back(atof(stream->work));
  
  // Read the distance
  
  readItem(stream);
  dist_.push_back(atof(stream->work));
  
}

void ModelReader::readItem(InputStream* stream)
{
  if(input_word(stream, 0, 1)) 
    ThrowError("Error in input_word()");
  
  if(input_skip_white(stream, 1, 0))
    ThrowError("Error in input_skip_space()");
}

/**.......................................................................
 * Find the nearest point to the passed MJD
 */
void ModelReader::fillInterpContainers(double mjd)
{
  unsigned size = mjd_.size();
  unsigned iStart=0, iStop=0;

  if(size == 0)
    ThrowError("No dates have been read in");
  
  // If the mjd is before the first date, fill in with the three first values
  
  if(mjd <= mjd_[0]) {
    iStart = 0;
    iStop  = (size > 2 ? 2 : size-1);
  } else if(mjd >= mjd_[size-1]) {
    iStop  = size-1;
    iStart = (size > 2 ? size-3 : 0);
  } else {
    
    // Else binary search for the bracketing pair, and return the nearest one.

    unsigned lo = 0, mi = size/2, hi = size-1;
    
    while (hi-lo > 1) {
      
      if(mjd > mjd_[mi]) 
	lo = mi;
      else
	hi = mi;
      
      mi = (hi+lo)/2;
    }
    
    // If the point is nearest to the lower bound, go one element
    // lower, unless there aren't any
    
    if(fabs(mjd-mjd_[lo]) < fabs(mjd-mjd_[hi])) {
      iStart = (lo == 0 ? lo : lo-1);
      iStop  = hi;
    } else {
      iStart = lo;
      iStop  = (hi == size-1 ? hi : hi+1);
    }
  }

  // Fill the interpolation containers

  distInterp_->empty();

  for(unsigned i=iStart; i <= iStop; i++) {

    distInterp_->extend(mjd_[i], dist_[i]);
  }

}

/**.......................................................................
 *  Get the distance at a given time.
 */
double ModelReader::getDistance(TimeVal& time, unsigned int& error)
{
  double mjd = time.getMjd();
  double finalVal;

  error = ERR_NONE;
  fillInterpContainers(mjd);

  // If this time stamp is outside the range which can be bracketed,
  // flag it as such

  if(!distInterp_->canBracket(mjd))
    error |= ERR_OUTSIDE_MJD;

  
  finalVal = distInterp_->evaluate(mjd);

  return finalVal;
}


/**.......................................................................
 *  Get the distance at a given time.
 */
double ModelReader::getDistance(double mjd, unsigned int& error)
{
  double finalVal;

  error = ERR_NONE;
  fillInterpContainers(mjd);

  // If this time stamp is outside the range which can be bracketed,
  // flag it as such

  if(!distInterp_->canBracket(mjd))
    error |= ERR_OUTSIDE_MJD;

  
  finalVal = distInterp_->evaluate(mjd);

  return finalVal;
}

