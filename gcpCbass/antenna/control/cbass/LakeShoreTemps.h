#ifndef LAKESHORETEMPS_H
#define LAKESHORETEMPS_H
/**
 * @file LsThermal.h
 * 
 * Tagged: Thu Oct 18 15:49:12 PDT 2007
 * 
 * @author GCP data acquisition
 */
#include "gcp/util/common/LsThermal.h"
#include "gcp/antenna/control/specific/Board.h"
#include "gcp/control/code/unix/libunix_src/common/tcpip.h"
#include "gcp/control/code/unix/libunix_src/common/regmap.h"


namespace gcp { 
  namespace antenna {
    namespace control {
      
      // Class to communciate with the LakeShore 218 
      
      class SpecificShare;
      
      class LakeShoreTemps : public Board, public gcp::util::LsThermal  { 
	
      public:
	
	/**
	 * Constructor.
	 */
	LakeShoreTemps();
	LakeShoreTemps(SpecificShare* share, std::string name);
	
	/**
	 * Destructor.
	 */
	virtual ~LakeShoreTemps();
	
	/**
	 * Requests all Temperatures
	 */
	void requestAllTemperatures();
	
	/**
	 *  Data registers we store
	 */
	RegMapBlock* tempSensors_;  
	
      }; // End class LakeShoreTemps
    } // End namespace control
  } // End namespace antenna
} // End namespace gcp
#endif // End #ifndef LAKESHORETEMPS_H
