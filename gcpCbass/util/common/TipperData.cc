#include "gcp/util/common/TipperData.h"

#include <iostream>
#include <iomanip>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
TipperData::TipperData() 
{
  NETSTRUCT_UINT(mjdDays_);
  NETSTRUCT_UINT(mjdMs_);
  NETSTRUCT_DOUBLE(tHot_);
  NETSTRUCT_DOUBLE(tWarm_);
  //  NETSTRUCT_DOUBLE(tAmb_);
  NETSTRUCT_DOUBLE(tChop_);
  //  NETSTRUCT_DOUBLE(tInt_);
  NETSTRUCT_DOUBLE(tSnork_);
  NETSTRUCT_DOUBLE(tAtm_);
  NETSTRUCT_DOUBLE(tau_);
  NETSTRUCT_DOUBLE(tSpill_);
  NETSTRUCT_DOUBLE(r_);
  NETSTRUCT_DOUBLE(mse_);
}

/**.......................................................................
 * Destructor.
 */
TipperData::~TipperData() {}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
ostream& 
gcp::util::operator<<(ostream& os, TipperData& data)
{
  TimeVal utc;
  utc.setMjd(data.mjdDays_, data.mjdMs_);

  os << utc 
     << " " << setw(8) << setprecision(5) << data.tHot_   
     << " " << setw(8) << setprecision(5) << data.tWarm_ 
     << " " << setw(8) << setprecision(5) << data.tChop_ 
     << " " << setw(8) << setprecision(5) << data.tSnork_ 
     << " " << setw(8) << setprecision(5) << data.tAtm_  
     << " " << setw(8) << setprecision(5) << data.tau_ 
     << " " << setw(8) << setprecision(5) << data.r_      
     << " " << setw(8) << setprecision(5) << data.mse_;
    
  return os;
}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
std::string TipperData::header()
{
  ostringstream os;

  os << setw(24) << "UTC"
     << " " << setw(8) << "THOT"
     << " " << setw(8) << "TWARM"
     << " " << setw(8) << "TCHOP" 
     << " " << setw(8) << "TSNORK"
     << " " << setw(8) << "TATM"  
     << " " << setw(8) << "TAU" 
     << " " << setw(8) << "R"      
     << " " << setw(8) << "MSE";

  return os.str();
}
