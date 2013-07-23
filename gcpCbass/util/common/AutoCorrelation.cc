#include "gcp/util/common/AutoCorrelation.h"

#include<iostream>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
AutoCorrelation::AutoCorrelation(int n, bool optimize) :
  Correlation(n, optimize)
{}

/**.......................................................................
 * Assignment Operator.
 */
void AutoCorrelation::operator=(AutoCorrelation& objToBeAssigned)
{
  std::cout << "Calling default assignment operator for class: AutoCorrelation" << std::endl;
};

/**.......................................................................
 * Output Operator.
 */
std::ostream& gcp::util::operator<<(std::ostream& os, AutoCorrelation& obj)
{
  os << "Default output operator for class: AutoCorrelation" << std::endl;
  return os;
};

/**.......................................................................
 * Destructor.
 */
AutoCorrelation::~AutoCorrelation() {}

void AutoCorrelation::pushSample(double sample)
{
  dft1_.pushSample(sample);
}
