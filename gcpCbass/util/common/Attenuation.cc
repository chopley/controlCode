#define __FILEPATH__ "util/common/Attenuation.cc"

#include "gcp/util/common/Attenuation.h"

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
Attenuation::Attenuation() 
{
  initialize();
}

Attenuation::Attenuation(const dBUnit& units, double dB)
{
  setdB(dB);
}

/**.......................................................................
 * Destructor.
 */
Attenuation::~Attenuation() {}

void Attenuation::setdB(double dB)
{
  dB_ = dB;
}

void Attenuation::initialize()
{
  setdB(0.0);
}

