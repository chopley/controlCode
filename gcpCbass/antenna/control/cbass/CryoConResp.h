#ifndef CRYOCONRESP_H
#define CRYOCONRESP_H
/**
 * @file LsThermal.h
 * 
 * Tagged: Thu Oct 18 15:49:12 PDT 2007
 * 
 * @author GCP data acquisition
 */
#include "gcp/util/common/CryoCon.h"
#include "gcp/antenna/control/specific/Board.h"
#include "gcp/control/code/unix/libunix_src/common/tcpip.h"
#include "gcp/control/code/unix/libunix_src/common/regmap.h"


namespace gcp { 
  namespace antenna {
    namespace control {
      
      // Class to record the output values from the CryoCon 
      
      class SpecificShare;
      
      class CryoConResp : public Board, public gcp::util::CryoCon  { 
	
      public:
	
	/**
	 * Constructor.
	 */
	CryoConResp();
	CryoConResp(SpecificShare* share, std::string name);
	
	/**
	 * Destructor.
	 */
	virtual ~CryoConResp();
	
	/**
	 * Requests load Temperature and writes it to disk
	 */
	void requestLoadTemperature();

	/**
	 * Requests heater output and writes it to disk
	 */
	void requestHeaterOutput();
	
	/**
	 *  Data registers we store
	 */
	RegMapBlock* tempSensor_;  
	RegMapBlock* heaterCurrent_;  
	
      }; // End class CryoConResp
    } // End namespace control
  } // End namespace antenna
} // End namespace gcp
#endif // End #ifndef CRYOCONRESP_H
