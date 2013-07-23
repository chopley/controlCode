#include "gcp/util/common/Angle.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

#include "gcp/control/code/unix/libunix_src/common/input.h"
#include "gcp/control/code/unix/libunix_src/common/output.h"

using namespace std;
using namespace gcp::util;

const double Angle::pi_               = M_PI;
const double Angle::twoPi_            = 2*M_PI;
const double Angle::degPerRad_        = 180/M_PI;
const double Angle::arcSecPerDegree_  = 3600;
const double Angle::masPerDegree_     = Angle::arcSecPerDegree_ * 1000;
const double Angle::arcSecPerRad_     = 206265;
const double Angle::arcMinPerRad_     = 60 * Angle::degPerRad_;
const double Angle::masPerRad_        = Angle::masPerDegree_ * Angle::degPerRad_;

/**.......................................................................
 * Constructor.
 */
Angle::Angle(bool modulo) 
{
  modulo_ = modulo;
  initialize();
}

/**.......................................................................
 * Constructor.
 */
Angle::Angle(const Radians& units, double radians, bool modulo) 
{
  modulo_ = modulo;
  setRadians(radians);
}

Angle::Angle(std::string degrees, bool modulo) 
{
  modulo_ = modulo;
  setRadians(sexagesimalToDouble(degrees) / degPerRad_);
}

Angle::Angle(const Degrees& units, double degrees, bool modulo) 
{
  modulo_ = modulo;
  setDegrees(degrees);
}

Angle::Angle(const ArcSec& units, double as, bool modulo) 
{
  modulo_ = modulo;
  setArcSec(as);
}

Angle::Angle(const MilliArcSec& units, double mas, bool modulo) 
{
  modulo_ = modulo;
  setMas(mas);
}

Angle::Angle(const ArcMinutes& units, double am, bool modulo) 
{
  modulo_ = modulo;
  setArcMinutes(am);
}

/**.......................................................................
 * Destructor.
 */
Angle::~Angle() {}

Angle::Angle(Angle& angle)
{
  *this = angle;
}

Angle::Angle(const Angle& angle)
{
  *this = (Angle&)angle;
}

void Angle::operator=(const Angle& angle)
{
  *this = (Angle&)angle;
}

void Angle::operator=(Angle& angle)
{
  radians_ = angle.radians_;
}

/**.......................................................................
 * Set the contents of this object
 */
void Angle::setRadians(double radians)
{
  radians_ = 0.0;
  addRadians(radians);
}

/**.......................................................................
 * Set the contents of this object
 */
void Angle::setDegrees(double degrees)
{
  setRadians(degrees / degPerRad_);
}

void Angle::setDegrees(double degrees, double arcmin, double arcsec)
{
  int sign=1;

  if(degrees < 0) {
    degrees = -degrees;
    sign = -1;
  }

  double deg = degrees + (arcmin + arcsec/60)/60;

  deg *= sign;

  setDegrees(deg);
}

/**.......................................................................
 * Set the contents of this object
 */
void Angle::setDegrees(std::string degrees)
{
  return setDegrees(sexagesimalToDouble(degrees));
}

/**.......................................................................
 * Set the contents of this object in arcminutes
 */
void Angle::setArcMinutes(double am)
{
  setRadians(am / arcMinPerRad_);
}

/**.......................................................................
 * Set the contents of this object in arcseconds
 */
void Angle::setArcSec(double as)
{
  setRadians(as / arcSecPerRad_);
}

/**.......................................................................
 * Set the contents of this object
 */
void Angle::setMas(double mas)
{
  setRadians(mas / masPerRad_);
}

/**.......................................................................
 * Add to the contents of this object
 */
void Angle::addRadians(double radians)
{
  radians_ += radians;

  if(modulo_) {
    while(radians_ > twoPi_)
      radians_ -= twoPi_;
    while(radians_ < 0.0)
      radians_ += twoPi_;
  }
}

/**.......................................................................
 * Add to the contents of this object
 */
void Angle::addDegrees(double degrees)
{
  addRadians(degrees / degPerRad_);
}

/**.......................................................................
 * Set the contents of this object
 */
void Angle::addDegrees(std::string degrees)
{
  return addDegrees(sexagesimalToDouble(degrees));
}

/**.......................................................................
 * Get the contents of this object
 */
std::string Angle::strDegrees()
{
  return doubleToSexagesimal(radians_ * degPerRad_);
}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
ostream& 
gcp::util::operator<<(ostream& os, Angle& angle)
{
  os << angle.strDegrees();
  return os;
}

/**.......................................................................
 * Convert a sexagesimal string to a double representation
 */
double Angle::sexagesimalToDouble(std::string valStr)
{
  LogStream errStr;
  InputStream* stream=0;
  double val;
  
  // Create an input stream.
  
  stream = new_InputStream();
  
  if(stream==0) {
    errStr.appendMessage(true, "Unable to allocate a new input stream\n");
    throw Error(errStr);
  }
  
  // Connect the catalog file to an input stream and read scans from
  // it into the catalog.
  
  try {
    
    if(open_StringInputStream(stream, 1, (char*)valStr.c_str())) {
      errStr.initMessage(true);
      errStr << "Unable to connect input stream to string: "
	     << valStr << endl;
      throw Error(errStr);
    }
    
    if(input_sexagesimal(stream, 0, &val)) {
      errStr.initMessage(true);
      errStr << "Error converting string: " << valStr << endl;
      throw Error(errStr);
    }
  } catch(Exception& err) {
    
    // Close the stream
    
    if(stream != 0)
      del_InputStream(stream);
    throw err;
  }
  
  // Close the stream
  
  if(stream != 0)
    del_InputStream(stream);
  
  if(errStr.isError())
    throw Error(errStr);
  
  return val;
}

/**.......................................................................
 * Convert a double to a sexagesimal string
 */
std::string Angle::doubleToSexagesimal(double val)
{
  LogStream errStr;
  OutputStream* stream=0;
  char valStr[100];
  
  // Create an input stream.
  
  stream = new_OutputStream();
  
  if(stream==0) {
    errStr.appendMessage(true, "Unable to allocate a new input stream\n");
    throw Error(errStr);
  }
  
  // Connect it to the string
  
  try {
    
    if(open_StringOutputStream(stream, 1, valStr, 100)) {
      errStr.initMessage(true);
      errStr << "Unable to connect output stream to string: "
	     << valStr << endl;
      throw Error(errStr);
    }
    
    if(output_sexagesimal(stream, " ", 0, 2, 2, val)) {
      errStr.initMessage(true);
      errStr << "Error converting string: " << valStr << endl;
      throw Error(errStr);
    }
  } catch(Exception& err) {
    
    // Close the stream
    
    if(stream != 0)
      del_OutputStream(stream);
    throw err;
  }
  
  // Close the stream
  
  if(stream != 0)
    del_OutputStream(stream);
  
  if(errStr.isError())
    throw Error(errStr);
  
  return std::string(valStr);
}

/**.......................................................................
 * Addition operator for Angle.
 */
Angle Angle::operator+(Angle& angle)
{
  Angle sum;
  sum.addRadians(radians_);
  sum.addRadians(angle.radians());
  return sum;
}

/**.......................................................................
 * Subtraction operator for Angle.
 */
Angle Angle::operator-(Angle& angle)
{
  Angle diff;
  diff.addRadians(radians_);
  diff.addRadians(-angle.radians());
  return diff;
}

void Angle::operator/=(unsigned uval)
{
  radians_ /= uval;
}

Angle Angle::operator/(unsigned uval)
{
  Angle div;
  div.setRadians(radians_ / uval);
  return div;
}

/**.......................................................................
 * Comparison operators
 */
bool Angle::operator>(Angle& angle)
{
  return radians_ > angle.radians();
}

bool Angle::operator>(const Angle& angle)
{
  return operator>((Angle&) angle);
}

bool Angle::operator>=(Angle& angle)
{
  return radians_ >= angle.radians();
}

bool Angle::operator<(Angle& angle)
{
  return radians_ < angle.radians();
}

bool Angle::operator<(const Angle& angle)
{
  return operator<((Angle&) angle);
}

bool Angle::operator<=(Angle& angle)
{
  return radians_ <= angle.radians();
}

void Angle::initialize()
{
  setRadians(0.0);
}

Angle::Angle(double radians, bool modulo)
{
  std::cout << "Value constructor called - very naughty" << std::endl;

  modulo_ = modulo;
  setRadians(radians);
}

