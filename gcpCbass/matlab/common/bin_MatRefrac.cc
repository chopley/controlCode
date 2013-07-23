/**.......................................................................
 * MATLAB Mex file for reading from GCP archive data files.
 *
 * Use like:
 *
 * d=gcpMatFastReadArc({'array.frame.record','corr.band0.usb[0][0]',
 *                     'antenna*.tracker.actual double', 'antenna*.tracker.source string'},
 *                     '06-jan-2005:15','06-jan-2005:16',
 *                     '/data/gcpdaq/arc','/home/gcpdaq/carma_unstable/gcp/array/conf/cal');
 *
 */
#include "gcp/control/code/unix/libmonitor_src/monitor_stream.h"

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Atmosphere.h"
#include "gcp/util/common/Monitor.h"
#include "gcp/util/common/RegDescription.h"
#include "gcp/util/common/RegParser.h"

#include "gcp/matlab/common/MexHandler.h"
#include "gcp/matlab/common/MexParser.h"

#include "mex.h"
#include "matrix.h"

#include <iostream>
#include <math.h>

using namespace std;
using namespace gcp::util;
using namespace gcp::matlab;

/**.......................................................................
 * Entry point from the matlab environment
 */
void mexFunction(int nlhs, mxArray      *plhs[],
		 int nrhs, const mxArray *prhs[])
{
  gcp::util::Logger::installStdoutPrintFn(MexHandler::stdoutPrintFn);
  gcp::util::Logger::installStderrPrintFn(MexHandler::stderrPrintFn);
  
  gcp::util::ErrHandler::installThrowFn(MexHandler::throwFn);
  gcp::util::ErrHandler::installReportFn(MexHandler::reportFn);
  gcp::util::ErrHandler::installLogFn(MexHandler::logFn);
  
  // Check input/output arguments

  //  MexHandler::checkArgs(2, 4, nlhs, nrhs);

  // Get the input frequency

  MexParser temp(prhs[0]);
  double* temperature = temp.getDoubleData();

  MexParser rh(prhs[1]);
  double* relhum = rh.getDoubleData();

  MexParser press(prhs[2]);
  double* pressure = press.getDoubleData();

  // Default to optical frequency (0.57 microns)

  double opticalFrequencyGHz = (3e8/(0.57*1e-6)/1e9);
  double* frequency = &opticalFrequencyGHz;
  int nFreq=1;

  if(nrhs==4) {
    MexParser freq(prhs[3]);
    frequency = freq.getDoubleData();
    nFreq = freq.getMaxDimension();
  }

  if(temp != rh || temp != press)
    ThrowError("Input arrays have incompatible dimensions");

  COUT("frequency = " << *frequency); 

  // Assign a return mxArray and return a double pointer to it

  double* a = MexHandler::createDoubleArray(&plhs[0], temp.getNumberOfDimensions(), temp.getDimensions());
  double* b = MexHandler::createDoubleArray(&plhs[1], temp.getNumberOfDimensions(), temp.getDimensions());

  gcp::util::Atmosphere atmos;

  Angle latitude;
  latitude.setDegrees(-90.0);

  atmos.setLatitude(latitude);

  Length altitude;
  altitude.setMeters(2843);

  atmos.setAltitude(altitude); 

  Frequency fq;
  Temperature tm;
  Pressure ps;
  Percent hd;

  int n = temp.getMaxDimension();

  for(unsigned i=0; i < n; i++) {

    fq.setGHz(nFreq > 1 ? *(frequency+i) : *frequency);
    tm.setC(*(temperature+i));
    hd.setPercentMax1(*(relhum+i));
    ps.setMilliBar(*(pressure+i));

    atmos.setFrequency(fq);
    atmos.setAirTemperature(tm);
    atmos.setHumidity(hd);
    atmos.setPressure(ps);

    gcp::util::Atmosphere::RefractionCoefficients coeffs = atmos.refractionCoefficients();

    *(a+i) = coeffs.a;
    *(b+i) = coeffs.b;

  }
    
  return;
}
