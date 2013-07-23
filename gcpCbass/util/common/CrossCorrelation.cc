#include "gcp/util/common/CrossCorrelation.h"
#include "gcp/util/common/Exception.h"

#include<iostream>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
CrossCorrelation::CrossCorrelation(int n, bool optimize) :
  Correlation(n, optimize), dft2_(n, optimize)
{
  correlationCalculated_ = false;
}

/**.......................................................................
 * Const Assignment Operator.
 */
void CrossCorrelation::operator=(const CrossCorrelation& objToBeAssigned)
{
  *this = (CrossCorrelation&)objToBeAssigned;
};

/**.......................................................................
 * Assignment Operator.
 */
void CrossCorrelation::operator=(CrossCorrelation& objToBeAssigned)
{
  std::cout << "Calling default assignment operator for class: CrossCorrelation" << std::endl;
};

/**.......................................................................
 * Output Operator.
 */
std::ostream& gcp::util::operator<<(std::ostream& os, CrossCorrelation& obj)
{
  os << "Default output operator for class: CrossCorrelation" << std::endl;
  return os;
};

/**.......................................................................
 * Destructor.
 */
CrossCorrelation::~CrossCorrelation() {}

/**.......................................................................
 * Compute the transform
 */
void CrossCorrelation::computeTransform()
{
  // Call the base-class method to calculate the first transform

  Correlation::computeTransform();

  // And calculate the second here

  dft2_.computeTransform();

  // Set the correlation calculated flag to false

  correlationCalculated_ = false;
}

/**.......................................................................
 * Return a pointer to the transformed data
 */
fftw_complex* CrossCorrelation::getTransform()
{
  COUT("Inside CrossCorrealtion::getTransform()");

  // If we haven't already calculated the cross-correlation from the
  // two transforms do it now

  if(!correlationCalculated_)
    calcCorrelation();

  // The result of the cross-correlation is stored in the transform
  // array of the first dft

  return dft1_.getTransform();
}

/**.......................................................................
 * Return true if the input array is full
 */
bool CrossCorrelation::isReadyForTransform()
{
  //  return dft1_.isReadyForTransform() && dft2_.isReadyForTransform();
  return false;
}

/**.......................................................................
 * Return a pointer to the transformed data
 */
fftw_complex* CrossCorrelation::calcCorrelation()
{
  COUT("Inside calCorrlation");

  fftw_complex* trans1 = dft1_.getTransform();
  fftw_complex* trans2 = dft2_.getTransform();

  double re1, im1, re2, im2;

  for(unsigned i; i < dft1_.transformSize(); i++) {
    re1 = trans1[i][0];
    im1 = trans1[i][1];

    re2 = trans2[i][0];
    im2 = trans2[i][1];

    trans1[i][0] = re1*re2 + im1*im2;
    trans1[i][1] = re1*im2 - im1*re2;
  }
}

/**.......................................................................
 * Push the next sample onto the input array
 */
void CrossCorrelation::pushSample(double sample1, double sample2)
{
  dft1_.pushSample(sample1);
  dft2_.pushSample(sample2);
}
