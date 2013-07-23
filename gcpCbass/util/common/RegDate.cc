#define __FILEPATH__ "util/common/RegDate.cc"

#include "gcp/util/common/RegDate.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

#include "gcp/control/code/unix/libunix_src/common/output.h"
#include "gcp/control/code/unix/libunix_src/common/astrom.h"

#include <math.h>

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
RegDate::RegDate(unsigned dayNo, unsigned mSec) 
{ 
  setDate(dayNo, mSec);
}

RegDate::RegDate(RegDate::Data& data)
{
  setDate(data.dayNo_, data.mSec_);
}

RegDate::RegDate(TimeVal& timeVal)
{
  setDate(timeVal.getMjdDays(), timeVal.getMjdMilliSeconds());
}

RegDate::RegDate() 
{
  setToCurrentTime();
}

void RegDate::operator=(const RegDate& date)
{
  setDate(date.data_.dayNo_, date.data_.mSec_);
}

void RegDate::operator=(RegDate& date)
{
  setDate(date.data_.dayNo_, date.data_.mSec_);
}

void RegDate::operator=(RegDate::Data& data)
{
  setDate(data.dayNo_, data.mSec_);
}

void RegDate::operator=(TimeVal& tVal)
{
  setDate(tVal.getMjdDays(), tVal.getMjdMilliSeconds(true));
}

void RegDate::setToCurrentTime() 
{
  timeVal_.setToCurrentTime();
  setDate(timeVal_.getMjdDays(),timeVal_.getMjdMilliSeconds());
}

void RegDate::setMjd(double mjd, bool doRound)
{
  unsigned dayNo = (unsigned)mjd;
  unsigned mSec;

  if(!doRound)
    mSec  = (unsigned) ((mjd - dayNo) * milliSecondsPerDay_);
  else {
    mSec  = (unsigned) rint(((mjd - dayNo) * milliSecondsPerDay_));
  }

  setDate(dayNo, mSec);
}

void RegDate::initialize()
{
  setDate(0,0);
}

void RegDate::setDate(unsigned dayNo, unsigned mSec) 
{
  setDayNumber(dayNo);
  setMilliSeconds(mSec);
}

void RegDate::setDayNumber(unsigned dayNo)
{
  data_.dayNo_ = dayNo;
}

void RegDate::setMilliSeconds(unsigned mSec)
{
  data_.mSec_ = mSec;
}

/**.......................................................................
 * Destructor.
 */
RegDate::~RegDate() {}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
ostream& 
gcp::util::operator<<(ostream& os, RegDate::Data& data)
{
  os << "day " << data.dayNo_ << " msec " << data.mSec_;
  return os;
}

/**.......................................................................
 * An output operator
 */
ostream& gcp::util::operator<<(ostream& os, RegDate& date)
{
  OutputStream* outputStream = 0;
  char fmtString[100];

  // Attempt to allocate the new output stream

  outputStream = new_OutputStream();

  if(!outputStream ||
     open_StringOutputStream(outputStream, 1, fmtString, 
			     sizeof(fmtString))) {
    del_OutputStream(outputStream);

    ThrowError("Unable to allocate a new stream");
  };

  if(gcp::control::output_utc(outputStream, "", 24, 3, date.mjd())==1) {
    del_OutputStream(outputStream);
    ThrowError("Error outputting date");
  }

  os << fmtString;

  del_OutputStream(outputStream);

  return os;
}

/**.......................................................................
 * An output operator
 */
std::string RegDate::str()
{
  OutputStream* outputStream = 0;
  char fmtString[100];

  // Attempt to allocate the new output stream

  outputStream = new_OutputStream();

  if(!outputStream ||
     open_StringOutputStream(outputStream, 1, fmtString, 
			     sizeof(fmtString))) {
    del_OutputStream(outputStream);

    ThrowError("Unable to allocate a new stream");
  };

  if(gcp::control::output_utc(outputStream, "", 24, 3, mjd())==1) {
    del_OutputStream(outputStream);
    ThrowError("Error outputting date");
  }

  std::string outputStr(fmtString);

  del_OutputStream(outputStream);

  return outputStr;
}

/**.......................................................................
 * Return the date, in MJD days
 */
double RegDate::mjd()
{
  return data_.dayNo_ + (double)data_.mSec_/(1000 * 86400);
}

/**.......................................................................
 * Return the time, in hours
 */
double RegDate::timeInHours()
{
  return (double)data_.mSec_ / (1000 * 3600);
}

/**.......................................................................
 * Return the fractional part of the time, in seconds
 */
double RegDate::timeInSeconds()
{
  return (double)data_.mSec_ / (1000);
}

bool RegDate::operator==(RegDate& date)
{
  return data_.dayNo_ == date.data_.dayNo_ &&
    data_.mSec_ == date.data_.mSec_;
}

bool RegDate::operator>(RegDate& date)
{
  return mjd() > date.mjd();
}

bool RegDate::operator>=(RegDate& date)
{
    return mjd() >= date.mjd();
}

bool RegDate::operator<(RegDate& date)
{
  return mjd() < date.mjd();  
}

bool RegDate::operator<=(RegDate& date)
{
  return mjd() <= date.mjd();
}

RegDate RegDate::operator+(const RegDate& date)
{
  RegDate sum(date);
  sum.data_.dayNo_ += date.data_.dayNo_;
  sum.data_.mSec_  += date.data_.mSec_;

  if(sum.data_.mSec_ > milliSecondsPerDay_) {
    sum.data_.dayNo_++;
    sum.data_.mSec_ = sum.data_.mSec_ % milliSecondsPerDay_;
  }
  return sum;
}

RegDate RegDate::operator-(const RegDate& date)
{
  long int dayDiff  = data_.dayNo_ - date.data_.dayNo_;
  long int mSecDiff = data_.mSec_  - date.data_.mSec_;

  if(mSecDiff < 0) {
    --dayDiff;
    mSecDiff += milliSecondsPerDay_;
  }
    
  RegDate diff((dayDiff < 0 ? 0 : dayDiff), mSecDiff);
  return diff;
}

void RegDate::operator+=(const RegDate& date)
{
  data_.dayNo_ += date.data_.dayNo_;
  data_.mSec_  += date.data_.mSec_;

  if(data_.mSec_ > milliSecondsPerDay_) {
    data_.dayNo_++;
    data_.mSec_ = data_.mSec_ % milliSecondsPerDay_;
  }
}

void RegDate::operator-=(const RegDate& date)
{
  long int dayDiff  = data_.dayNo_ -= date.data_.dayNo_;
  long int mSecDiff = data_.mSec_  -= date.data_.mSec_;

  if(mSecDiff < 0) {
    --dayDiff;
    mSecDiff += milliSecondsPerDay_;
  }
    
  data_.dayNo_ = (dayDiff < 0 ? 0 : dayDiff);
  data_.mSec_  = mSecDiff;
}

RegDate RegDate::operator/(unsigned int divisor)
{
  double fracDay = data_.dayNo_ + ((double)data_.mSec_)/milliSecondsPerDay_;
  fracDay /= divisor;

  RegDate div((unsigned)fracDay, 
	      (unsigned)((fracDay-(unsigned)fracDay) * milliSecondsPerDay_));
  return div;
}

void RegDate::Data::operator+=(unsigned mSec)
{
  mSec_ += mSec;

  if(mSec_ > milliSecondsPerDay_) {
    dayNo_++;
    mSec_ = mSec_ % milliSecondsPerDay_;
  }
}

void RegDate::Data::operator=(TimeVal& timeVal)
{
  dayNo_ = timeVal.getMjdDays();
  mSec_  = timeVal.getMjdMilliSeconds();
}

void RegDate::updateFromTimeVal()
{
  data_.dayNo_ = timeVal_.getMjdDays();
  data_.mSec_  = timeVal_.getMjdMilliSeconds();
}
