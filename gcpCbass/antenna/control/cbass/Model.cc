#define __FILEPATH__ "antenna/control/specific/Model.cc"

#include <cmath>

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/Debug.h"

#include "gcp/antenna/control/specific/Model.h"

#include "gcp/control/code/unix/libunix_src/common/const.h"

using namespace std;
using namespace gcp::antenna::control;
using namespace gcp::util;

/**.......................................................................
 * Constructor simply intializes each of the encoders
 */
Model::Model() : 
  az_(Axis::AZ), el_(Axis::EL), pa_(Axis::PA) 
{
  fixedOpticalCollimation_.resize(PointingTelescopes::nPtel_);
  opticalFlexure_.resize(PointingTelescopes::nPtel_);

  reset();
}

/**.......................................................................
 * Destructor.
 */
Model::~Model() {}

/**.......................................................................
 * Reset internal data members.
 */
void Model::reset()
{
  // Reset the encoder calibrations

  az_.reset();
  el_.reset();

  // Reset the tilts

  azt_.reset();
  elt_.reset();

  // Reset the collimation terms

  for(unsigned iPtel=0; iPtel < PointingTelescopes::nPtel_; iPtel++) {
    fixedOpticalCollimation_[iPtel].reset();
  }

  fixedRadioCollimation_.reset();

  collimationType_ = gcp::util::Collimation::FIXED;
  collimationMode_ = gcp::util::PointingMode::RADIO;
  collimationPtel_ = gcp::util::PointingTelescopes::PTEL_NONE;

  setCurrentCollimation();

  // Reset the flexure terms

  for(unsigned iPtel=0; iPtel < PointingTelescopes::nPtel_; iPtel++) 
    opticalFlexure_[iPtel].reset();

  radioFlexure_.reset();

  currentFlexure_ = &radioFlexure_;
}

/**.......................................................................
 * Return a pointer to the requested collimation container
 */
gcp::antenna::control::Collimation* 
Model::Collimation(PointingMode::Type mode, PointingTelescopes::Ptel ptel, gcp::util::Collimation::Type type)
{
  unsigned iPtel;

  switch (mode) {

  case PointingMode::OPTICAL:

    iPtel = PointingTelescopes::ptelToInt(ptel);
    
    switch (type) {
    case gcp::util::Collimation::FIXED:
      return &fixedOpticalCollimation_[iPtel];
      break;
    default:
      ThrowError("Invalid collimation type");
      break;
    }
    
    break;

  case PointingMode::RADIO:

    switch (type) {
    case gcp::util::Collimation::FIXED:
      return &fixedRadioCollimation_;
      break;
    default:
      ThrowError("Invalid collimation type");
      break;
    }

    break;
  default:
    ThrowError("Invalid collimation mode.\n");
    break;
  }
}

/**.......................................................................
 * Return a pointer to the current collimation model
 */
gcp::antenna::control::Collimation* Model::currentCollimation(gcp::util::Collimation::Type type)
{
  switch(type) {
  case gcp::util::Collimation::FIXED:
    return currentFixedCollimation_;
    break;
  default:
    ReportError("Invalid mode");
    break;
  }
}

/**.......................................................................
 * Return a pointer to the requested flexure container
 */
gcp::antenna::control::Flexure* 
Model::Flexure(PointingMode::Type mode, PointingTelescopes::Ptel ptel)
{
  switch (mode) {
  case PointingMode::OPTICAL:
    {
      unsigned iPtel = PointingTelescopes::ptelToInt(ptel);
      return &opticalFlexure_[iPtel];
    }
    break;
  case PointingMode::RADIO:
    return &radioFlexure_;
    break;
  default:
    throw Error("Model::Flexure: Unrecognized flexure mode.\n");
    break;
  }
}

/**.......................................................................
 * Return a pointer to the current flexure model
 */
gcp::antenna::control::Flexure* Model::currentFlexure()
{
  return currentFlexure_;
}

/**.......................................................................
 * Return a pointer to the requested encoder container
 */
gcp::antenna::control::Encoder* 
Model::Encoder(Axis::Type axis)
{
  switch (axis) {
  case Axis::AZ:
    return &az_;
    break;
  case Axis::EL:
    return &el_;
    break;
  case Axis::PA:
    return &pa_;
    break;
  default:
    throw Error("Model::Encoder: Unrecognized encoder axis.\n");
    break;
  }
}

/**.......................................................................
 * Compute and store the new mount limits as angles on the sky
 */
void Model::updateMountLimits()
{
  az_.updateMountLimits();
  el_.updateMountLimits();
}

/**.......................................................................
 * Return true if the passed collimation container is the current one.
 */
bool Model::isCurrent(gcp::antenna::control::Collimation* collim, gcp::util::Collimation::Type type)
{
  switch(type) {
  case gcp::util::Collimation::FIXED:
    return collim == currentFixedCollimation_;
    break;
  default:
    ReportError("Invalid mode");
    break;
  }
}

/**.......................................................................
 * Return true if the passed flexure container is the current one.
 */
bool Model::isCurrent(gcp::antenna::control::Flexure* flexure)
{
  return flexure == currentFlexure_;
}

/**.......................................................................
 * Set which pointing type (fixed or polar) is the current
 * collimation mode).
 */
void Model::setCollimationType(gcp::util::Collimation::Type type)
{
  collimationType_ = type;
  setCurrentCollimation();
}

/**.......................................................................
 * Set which pointing mode (optical or radio) is the current
 * collimation mode).
 */
void Model::setCurrentCollimation(PointingMode::Type mode, PointingTelescopes::Ptel ptel)
{
  collimationMode_ = mode;
  collimationPtel_ = ptel;
  setCurrentCollimation();
}

void Model::setCurrentCollimation()
{
  switch (collimationMode_) {
  case PointingMode::OPTICAL:
    {
      unsigned iPtel = PointingTelescopes::ptelToInt(collimationPtel_);
      currentFixedCollimation_ = &fixedOpticalCollimation_[iPtel];
    }
    break;
  case PointingMode::RADIO:
    currentFixedCollimation_ = &fixedRadioCollimation_;
    break;
  default:
    ReportError("Invalid collimation mode.\n");
    break;
  }
}

/**.......................................................................
 * Set which pointing mode (optical or radio) is the current
 * flexure mode).
 */
void Model::setCurrentFlexure(PointingMode::Type mode, PointingTelescopes::Ptel ptel)
{
  switch (mode) {
  case PointingMode::OPTICAL:
    {
      unsigned iPtel = PointingTelescopes::ptelToInt(collimationPtel_);
      currentFlexure_ = &opticalFlexure_[iPtel];
    }
    break;
  case PointingMode::RADIO:
    currentFlexure_ = &radioFlexure_;
    break;
  default:
    ErrorDef(err, "Model::setCurrentFlexure: Invalid flexure mode.\n");
    break;
  }
}

/**.......................................................................
 * Adjust the elevation to account for telescope flexure.
 *
 * Input/Output:
 *  f    PointingCorrections *  The elevation pointing to be corrected.
 */
void Model::applyFlexure(PointingCorrections* f)
{
  currentFlexure_->apply(f);
}

/**.......................................................................
 * Correct the collimation of the telescope.
 *
 * Input/Output:
 *  f       PointingCorrections *  The az/el pointing to be corrected.
 */
void Model::applyCollimation(PointingCorrections* f, TrackerOffset& offset)
{
  currentFixedCollimation_->apply(f, offset);
}

/**.......................................................................
 * Pack the zero points for encoders managed by this object
 */
void Model::packEncoderZeros(signed* s_elements) 
{
  az_.packZero(s_elements);
  el_.packZero(s_elements+1);
}

/**.......................................................................
 * Pack the multipliers for encoders managed by this object.
 */
void Model::packEncoderMultipliers(signed* s_elements)
{
  az_.packCountsPerTurn(s_elements);
  el_.packCountsPerTurn(s_elements+1);
}

/**.......................................................................
 * Pack the tilts managed by this object.
 */
void Model::packTilts(signed* s_elements)
{
  azt_.packHaTilt(s_elements);
  azt_.packLatTilt(s_elements+1);
  elt_.packTilt(s_elements+2);
}

/**.......................................................................
 * Pack the flexure term managed by this object.
 */
void Model::packFlexure(signed* s_elements)
{
  currentFlexure_->pack(s_elements);
}

/**.......................................................................
 * Pack which collimation mode is the current one.
 */
void Model::packCollimationMode(unsigned* u_elements)
{
  u_elements[0] = (collimationMode_ == gcp::util::PointingMode::RADIO);
}

/**.......................................................................
 * Pack which collimation mode is the current one.
 */
void Model::packCollimationType(unsigned* u_elements)
{
  u_elements[0] = collimationType_;
}

/**.......................................................................
 * Pack the current collimation correction.
 */
void Model::packCollimation(signed* s_elements, 
			    gcp::util::Collimation::Type type)
{
  switch(type) {
  case gcp::util::Collimation::FIXED:
    currentFixedCollimation_->pack(s_elements);
    break;
  default:
    ThrowError("Unrecognized collimation type");
    break;
  }
}

/**.......................................................................
 * Return a pointer to the requested collimation container
 */
gcp::antenna::control::AxisTilt* 
Model::AxisTilt(Axis::Type axis)
{
  switch (axis) {
  case Axis::AZ:
    return &azt_;
    break;
  case Axis::EL:
    return &elt_;
    break;
  default:
    ThrowError("Model::AxisTilt: Unrecognized axis.\n");
    break;
  }
}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
ostream& 
gcp::antenna::control::operator<<(ostream& os, Model& model)
{
  os << "collim (ptel0) : " << model.fixedOpticalCollimation_[0] << std::endl
     << "collim (ptel1) : " << model.fixedOpticalCollimation_[1] << std::endl
     << "collim (ptel2) : " << model.fixedOpticalCollimation_[2] << std::endl;

  os << std::endl;

  os << "flex (ptel0) : " << model.opticalFlexure_[0] << std::endl
     << "flex (ptel1) : " << model.opticalFlexure_[1] << std::endl
     << "flex (ptel2) : " << model.opticalFlexure_[2] << std::endl;

  return os;
}

gcp::util::PointingTelescopes::Ptel Model::getCollimationPtel()
{
  return collimationPtel_;
}

