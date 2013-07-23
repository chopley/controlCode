#include "gcp/util/common/Temperature.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

using namespace std;

using namespace gcp::util;

const double Temperature::kelvinZeroPointInC_ = 273.15;

/**.......................................................................
 * Constructor.
 */
Temperature::Temperature() 
{
  initialize();
}

/**.......................................................................
 * Constructors.
 */
Temperature::Temperature(const Kelvin& units, double kelvin) 
{
  initialize();
  setK(kelvin);
}

Temperature::Temperature(const Centigrade& units, double centigrade) 
{
  initialize();
  setC(centigrade);
}

Temperature::Temperature(const Celsius& units, double celsius) 
{
  initialize();
  setC(celsius);
}

Temperature::Temperature(const Fahrenheit& units, double fahrenheit) 
{
  initialize();
  setF(fahrenheit);
}

/**.......................................................................
 * Destructor.
 */
Temperature::~Temperature() {}

void Temperature::setC(double centigrade)
{
  centigrade_ = centigrade;
}

void Temperature::setF(double fahrenheit)
{
  centigrade_ = (fahrenheit - 32.0) * 5.0/9;
}

void Temperature::setK(double kelvin)
{
  centigrade_ = kelvin - kelvinZeroPointInC_;
}

double Temperature::C()
{
  return centigrade_;
}

double Temperature::F()
{
  return (9.0/5 * centigrade_ + 32.0);
}

double Temperature::K()
{
  return centigrade_ + kelvinZeroPointInC_;
}

void Temperature::initialize()
{
  setC(0.0);
}

void Temperature::Kelvin::addNames()
{
  COUT("Calling real addNames() method");

  addName("Kelvin");
  addName("kelvin");
}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
ostream& 
gcp::util::operator<<(ostream& os, Temperature& temp)
{
  os << temp.K() << " K";
  return os;
}
