#ifndef MODEL_H
#define MODEL_H

/**
 * @file Model.h
 * 
 * Tagged: Thu Nov 13 16:53:41 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Axis.h"
#include "gcp/util/common/Collimation.h"
#include "gcp/util/common/PointingMode.h"
#include "gcp/util/common/PointingTelescopes.h"

#include "gcp/antenna/control/specific/AzTilt.h"
#include "gcp/antenna/control/specific/FixedCollimation.h"
#include "gcp/antenna/control/specific/Flexure.h"
#include "gcp/antenna/control/specific/ElTilt.h"
#include "gcp/antenna/control/specific/Encoder.h"
#include "gcp/antenna/control/specific/TrackerOffset.h"

#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"

#include <vector>

namespace gcp {
  namespace antenna {
    namespace control {
      
      // Encapsulate the pointing model parameters.

      class Model {
	
      public:
	
	// Constructor.

	Model();
	
	// Destructor.

	~Model();
	
	// Reset internal data members

	void reset();
	
	// Return a pointer to the requested collimation model

	gcp::antenna::control::Collimation* 
	  Collimation(gcp::util::PointingMode::Type mode,
		      gcp::util::PointingTelescopes::Ptel ptel=gcp::util::PointingTelescopes::PTEL_NONE,
		      gcp::util::Collimation::Type type = 
		      gcp::util::Collimation::FIXED);
	
	// Set which pointing mode (optical or radio) is the current
	// collimation mode)

	void setCurrentCollimation(gcp::util::PointingMode::Type mode,
				   gcp::util::PointingTelescopes::Ptel ptel);
	
	// Set the right collimation for whatever type is set.

	void setCurrentCollimation();

	// Set the right collimation for whatever type is set.

	void setCollimationType(gcp::util::Collimation::Type type);
	
	// Return a pointer to the current collimation model

	gcp::antenna::control::Collimation* 
	  currentCollimation(gcp::util::Collimation::Type type);
	
	// Return a pointer to the requested flexure model

	gcp::antenna::control::Flexure* 
	  Flexure(gcp::util::PointingMode::Type mode,
		  gcp::util::PointingTelescopes::Ptel ptel=
		  gcp::util::PointingTelescopes::PTEL_NONE);
	
	// Set which pointing mode (optical or radio) is the current
	// flexure mode)

	void setCurrentFlexure(gcp::util::PointingMode::Type mode,
			       gcp::util::PointingTelescopes::Ptel ptel);
	
	// Return a pointer to the current flexure model

	gcp::antenna::control::Flexure* currentFlexure();
	
	// Return a pointer to the requested encoder model

	gcp::antenna::control::Encoder* 
	  Encoder(gcp::util::Axis::Type axis);
	
	// Compute and store the new mount limits as angles on the sky

	void updateMountLimits();
	
	// Set the flexure term.

	void setFlexure(double flexure);
	
	// Return true if the passed collimation container is the current one.
	
	bool isCurrent(gcp::antenna::control::Collimation* collim,
		       gcp::util::Collimation::Type type);
	
	// Return true if the passed flexure container is the current one.
	
	bool isCurrent(gcp::antenna::control::Flexure* flexure);

	// Adjust the elevation to account for telescope flexure.
	//
	//  PointingCorrections* f = The elevation pointing to be
	//  corrected.

	void applyFlexure(PointingCorrections* f);
	
	// Correct the collimation of the telescope.
	//
	// PointingCorrections* f = The az/el pointing to be
	// corrected.

	void applyCollimation(PointingCorrections* f, TrackerOffset& offset);
	
	// Return a pointer to the requested tilt

	gcp::antenna::control::AxisTilt* 
	  AxisTilt(gcp::util::Axis::Type axis);
	
	// Pack the zero points for encoders managed by this object

	void packEncoderZeros(signed* s_elements);
	
	// Pack the multipliers for encoders managed by this object.

	void packEncoderMultipliers(signed* s_elements);
	
	// Pack the tilts managed by this object.

	void packTilts(signed* s_elements);
	
	// Pack the flexure term managed by this object.

	void packFlexure(signed* s_elements);
	
	// Pack which collimation mode is the current one.

	void packCollimationMode(unsigned* u_elements);
	void packCollimationType(unsigned* u_elements);
	gcp::util::PointingTelescopes::Ptel getCollimationPtel();

	// Pack the current collimation correction.

	void packCollimation(signed* s_elements, 
			     gcp::util::Collimation::Type type);
	
	friend std::ostream& operator<<(std::ostream& os, Model& model);

      private:
	
	// The calibration of the azimuth encoder

	gcp::antenna::control::Encoder az_; 
	
	// The calibration of the elevation encoder

	gcp::antenna::control::Encoder el_; 
	
	// The calibration of the deck encoder

	gcp::antenna::control::Encoder pa_; 
	
	// The azimuth tilt

	AzTilt azt_;           
	
	// The elevation tilt

	ElTilt elt_;
	
	//------------------------------------------------------------
	// Collimation terms
	//------------------------------------------------------------

	gcp::util::Collimation::Type        collimationType_;
	gcp::util::PointingMode::Type       collimationMode_;
	gcp::util::PointingTelescopes::Ptel collimationPtel_;

	// The collimation models for optical pointing

	std::vector<gcp::antenna::control::FixedCollimation> 
	  fixedOpticalCollimation_; 

	// The collimation model for radio pointing

	gcp::antenna::control::FixedCollimation fixedRadioCollimation_;   
	
	// A pointer to the currently selected radio or or optical
	// collimation model

	gcp::antenna::control::Collimation* currentFixedCollimation_;
	
	//------------------------------------------------------------
	// Flexure terms
	//------------------------------------------------------------

	// The flexure models for optical pointing

	std::vector<gcp::antenna::control::Flexure> opticalFlexure_; 
	
	// The flexure model for radio pointing

	gcp::antenna::control::Flexure radioFlexure_;   
	
	// A pointer to the currently selected radio or or optical
	// flexure model

	gcp::antenna::control::Flexure* currentFlexure_;
	
      }; // End class Model
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
