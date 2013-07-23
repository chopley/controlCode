/**.......................................................................
 * MATLAB Mex file for accessing the mars model
 *
 */
#include <iostream>
#include <cmath>

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/ModelReader.h"
#include "gcp/util/common/Angle.h"
#include "gcp/matlab/common/MexHandler.h"
#include "gcp/antenna/control/specific/Site.cc"


#include "mex.h"
#include "matrix.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::matlab;
using namespace gcp::antenna::control;

void mexFunction(int nlhs, mxArray      *plhs[],
		 int nrhs, const mxArray *prhs[])
{
  gcp::util::Logger::installStdoutPrintFn(MexHandler::stdoutPrintFn);
  gcp::util::Logger::installStderrPrintFn(MexHandler::stderrPrintFn);
  
  gcp::util::ErrHandler::installThrowFn(MexHandler::throwFn);
  gcp::util::ErrHandler::installReportFn(MexHandler::reportFn);
  gcp::util::ErrHandler::installLogFn(MexHandler::logFn);

  if(nrhs != 2) {
    std::cerr << "Wrong number of arguments. " << std::endl
	      << "Should be: directory source mjd[N]" << std::endl;
    return;
  }
  
  

  // declarations
  double thisDistance = 0;
  unsigned int thisError = 0;
  
  
  // read in the source
  std::string dir("/home/cbass/gcpCbass/control/ephem");
  MexParser source(prhs[0]);
  std::string sourcename = source.getString();
  std::string fileend("_3yr.ephem");
  std::string filename = sourcename+fileend;

  // read in the model
  ModelReader model(dir, filename);
  
  // get the times to calculate
  MexParser mjd(prhs[1]);
  double* mjd_val = mjd.getDoubleData();
  int n = mjd.getMaxDimension();
  
  // assign the return mxArray and return a double pointer to it
  double* dist = MexHandler::createDoubleArray(&plhs[0], mjd.getNumberOfDimensions(), mjd.getDimensions());
  double* error = MexHandler::createDoubleArray(&plhs[1], mjd.getNumberOfDimensions(), mjd.getDimensions());
  
  for(unsigned i=0; i<n; i++){
    
    thisDistance = model.getDistance(*(mjd_val+i), thisError);
    *(dist+i)  = thisDistance;
    *(error+i) = (double) thisError;
  };

  // now for rcent (in AU)
  Site site;
  Angle lon, lat;
  double altitude = 1222;
  lon.setDegrees(-118.28222);
  lat.setDegrees(37.23388);
  site.setFiducial(lon, lat, altitude);


  COUT("rcent: " << site.actual_.rcent);
  double *rcent;
  // assign the last output
  plhs[2] = mxCreateDoubleMatrix(1,1,mxREAL);
  rcent = mxGetPr(plhs[2]);
  *rcent = site.actual_.rcent;

}
