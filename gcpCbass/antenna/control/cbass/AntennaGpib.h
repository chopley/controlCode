// $Id: AntennaGpib.h,v 1.4 2009/08/25 21:07:39 eml Exp $

#ifndef GCP_ANTENNA_CONTROL_ANTENNAGPIB_H
#define GCP_ANTENNA_CONTROL_ANTENNAGPIB_H

/**
 * @file AntennaGpib.h
 * 
 * Tagged: Tue Aug 18 12:13:04 PDT 2009
 * 
 * @version: $Revision: 1.4 $, $Date: 2009/08/25 21:07:39 $
 * 
 * @author tcsh: username: Command not found.
 */
#include "gcp/util/common/CryoCon.h"
#include "gcp/util/common/GpibUsbController.h"
#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/LsThermal.h"

#include "gcp/antenna/control/specific/AntennaGpibMsg.h"
#include "gcp/antenna/control/specific/SpecificTask.h"
#include "gcp/antenna/control/specific/Board.h"
#include "gcp/util/specific/Directives.h"
#include "gcp/control/code/unix/libunix_src/common/tcpip.h"
#include "gcp/control/code/unix/libunix_src/common/regmap.h"

#include <string>

namespace gcp {
  namespace antenna {
    namespace control {

      class AntennaControl;

      class SpecificShare;
      /**
       * AntennaGpib class will handle all gpib functions.  This class
       * instantiates objects for communicating with Cryocon and
       * Lakeshore thermal boxes.
       */

      class AntennaGpib : 
	public SpecificTask,
	public gcp::util::GenericTask<AntennaGpibMsg>{

      public:

	// Constructor.

	AntennaGpib(AntennaControl* parent=0, 
		    std::string device="whatever", 
		    unsigned cryoConAddr=12, 
		    unsigned lsThermoAddr=11);

	// Destructor.

	virtual ~AntennaGpib();

      private:

	// Board object to be able to write data

	Board* thermalBoard_;

	// Declare AntennaControl a friend so that the parent has
	// access to our private members

	friend class AntennaControl;

	// A pointer to the parent task resources

	AntennaControl* parent_;
	
	gcp::util::GpibUsbController* gpib_;
	gcp::util::CryoCon*           cryoCon_;
	gcp::util::LsThermal*         lsThermo_;

	std::string gpibDevice_;
	unsigned cryoConAddr_;
	unsigned lsThermoAddr_;

	// Attempt to connect to the GPIB controller

	bool connect();

	// Readout Lakeshore thermometry

	void readoutThermometry();

	// previous index in readout
	
	int previousIndex_;

	// Readout CryoCon heater and temperature sensor

	void readoutCryoCon();
	
	// Execute the GPIB command

	void executeGpibCmd(AntennaGpibMsg* msg);

	// Process a message received on our message queue

	void processMsg(AntennaGpibMsg* msg);
	
	//  Data registers we store

        RegMapBlock* utc_;
        RegMapBlock* lsTempSensors_;
	RegMapBlock* ccTempSensor_;
	RegMapBlock* ccHeaterCurrent_;

      }; // End class AntennaGpib

    } // End namespace control
  } // End namespace antenna
} // End namespace gcp



#endif // End #ifndef GCP_ANTENNA_CONTROL_ANTENNAGPIB_H
