#include "gcp/util/common/Correlation.h"
#include "gcp/util/common/Exception.h"

#include<iostream>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
Correlation::Correlation(int n, bool optimize) :
  dft1_(n, optimize)
{
  absCalculated_ = false;
}

/**.......................................................................
 * Const Assignment Operator.
 */
void Correlation::operator=(const Correlation& objToBeAssigned)
{
  *this = (Correlation&)objToBeAssigned;
};

/**.......................................................................
 * Assignment Operator.
 */
void Correlation::operator=(Correlation& objToBeAssigned)
{
  std::cout << "Calling default assignment operator for class: Correlation" << std::endl;
};

/**.......................................................................
 * Output Operator.
 */
std::ostream& gcp::util::operator<<(std::ostream& os, Correlation& obj)
{
  os << "Default output operator for class: Correlation" << std::endl;
  return os;
};

/**.......................................................................
 * Destructor.
 */
Correlation::~Correlation() {}

/**.......................................................................
 * Return the squared tranform
 */
double* Correlation::abs2()
{
  // If the absolute value hasn't yet been calculated, do it now

  if(!absCalculated_)
    calcAbs2();

  // The result is stored in the input array of the first dft

  return dft1_.getInputData();
}

/**.......................................................................
 * Calculate the squared transform
 */
void Correlation::calcAbs2()
{
  fftw_complex* trans = getTransform();
  double*       abs2  = dft1_.getInputData();

  unsigned n = dft1_.transformSize();

  for(unsigned i=0; i < n; i++)
    abs2[i] = trans[i][0] * trans[i][0] + trans[i][1] * trans[i][1];

  absCalculated_ = true;
}

/**.......................................................................
 * Compute the transform
 */
void Correlation::computeTransform()
{
  dft1_.computeTransform();

  absCalculated_ = false;
}

/**.......................................................................
 * Return a pointer to the transformed data
 */
fftw_complex* Correlation::getTransform()
{
  COUT("Inside Base-class::getTransform()");

  return dft1_.getTransform();
}

/**.......................................................................
 * Return true if the input array is full
 */
bool Correlation::isReadyForTransform()
{
  return false;
}

/**.......................................................................
 * Return the size of the transformed array
 */
unsigned Correlation::transformSize()
{
  return dft1_.transformSize();
}
