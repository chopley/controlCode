#include "gcp/util/common/CurlUtils.h"
#include "gcp/util/common/DliPowerStrip.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/String.h"

using namespace std;
using namespace gcp::util;

const unsigned DliPowerStrip::MAX_OUTLETS = 8;

/**.......................................................................
 * Constructor.
 */
DliPowerStrip::DliPowerStrip(std::string host) 
{
  host_ = host;
  state_.resize(MAX_OUTLETS);

  for(unsigned i=0; i < MAX_OUTLETS; i++) {
    state_[i] = UNKNOWN;
  }

}

/**.......................................................................
 * Destructor.
 */
DliPowerStrip::~DliPowerStrip() {}

void DliPowerStrip::checkOutlet(unsigned outlet)
{
  if(outlet > MAX_OUTLETS)
    ThrowError("Invalid outlet number: should be 0-" << MAX_OUTLETS);
}

void DliPowerStrip::cycleAll()
{
  std::ostringstream os;
  os << "http://admin:power@" << host_ << "/outlet?a=CCL";
  (void)sendRequest(os.str());
}

void DliPowerStrip::allOn()
{
  std::ostringstream os;
  os << "http://admin:power@" << host_ << "/outlet?a=ON";
  (void)sendRequest(os.str());
}

void DliPowerStrip::allOff()
{
  std::ostringstream os;
  os << "http://admin:power@" << host_ << "/outlet?a=OFF";
  (void)sendRequest(os.str());
}

void DliPowerStrip::cycle(unsigned outlet)
{
  checkOutlet(outlet);

  std::ostringstream os;
  os << "http://admin:power@" << host_ << "/outlet?" << outlet << "=CCL";
  (void)sendRequest(os.str());
}

void DliPowerStrip::cycle(Outlet outletMask)
{
  unsigned mask = (unsigned) outletMask;

  struct timespec ts;
  ts.tv_sec  =         0;
  ts.tv_nsec = 500000000;

  for(unsigned i=0; i < MAX_OUTLETS; i++) {
    if(mask & (1<<i)) {
      cycle(i+1);
      nanosleep(&ts, 0);
    }
  }
}

void DliPowerStrip::on(unsigned outlet)
{
  checkOutlet(outlet);

  std::ostringstream os;
  os << "http://admin:power@" << host_ << "/outlet?" << outlet << "=ON";
  (void)sendRequest(os.str());
}

void DliPowerStrip::on(Outlet outletMask)
{
  unsigned mask = (unsigned) outletMask;

  struct timespec ts;
  ts.tv_sec  =         0;
  ts.tv_nsec = 500000000;

  for(unsigned i=0; i < MAX_OUTLETS; i++) {
    if(mask & (1<<i)) {
      on(i+1);
      nanosleep(&ts, 0);
    }
  }
}

void DliPowerStrip::off(unsigned outlet)
{
  checkOutlet(outlet);

  std::ostringstream os;
  os << "http://admin:power@" << host_ << "/outlet?" << outlet << "=OFF";
  (void)sendRequest(os.str());
}

void DliPowerStrip::off(Outlet outletMask)
{
  unsigned mask = (unsigned) outletMask;

  struct timespec ts;
  ts.tv_sec  =         0;
  ts.tv_nsec = 500000000;

  for(unsigned i=0; i < MAX_OUTLETS; i++) {
    if(mask & (1<<i)) {
      off(i+1);
      nanosleep(&ts, 0);
    }
  }
}

std::string DliPowerStrip::sendRequest(std::string str)
{
  CurlUtils curl;
  return curl.getUrl(str, false);
}

std::vector<DliPowerStrip::State> DliPowerStrip::queryStatus()
{
  std::ostringstream os;
  os << "http://admin:power@" << host_ << "/index.htm";

  String status(sendRequest(os.str()));

  for(unsigned i=0; i < MAX_OUTLETS; i++) {

    status.findNextInstanceOf("font color=", true, "", false, false);
    String substr = status.findNextInstanceOf(">", true, "<", true, true);

    if(substr.str() == "OFF")
      state_[i] = OFF;
    else if(substr.str() == "ON")
      state_[i] = ON;
    else
      state_[i] = UNKNOWN;

  }

  return state_;
}
