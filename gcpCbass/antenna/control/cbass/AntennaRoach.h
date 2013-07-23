#ifndef GCP_ANTENNA_CONTROL_ANTENNAROACH_H
#define GCP_ANTENNA_CONTROL_ANTENNAROACH_H

/**
 * @file AntennaRoach.h
 * 
 * Tagged: Mon Nov 23 15:59:36 PST 2009
 * 
 */
#include "gcp/util/common/GenericTask.h"

#include "gcp/antenna/control/specific/RoachBackendMsg.h"
#include "gcp/antenna/control/specific/Backend.h"
#include "gcp/antenna/control/specific/SpecificTask.h"
#include "gcp/util/specific/Directives.h"
#include "gcp/util/common/String.h"

namespace gcp {
  namespace antenna {

    namespace control {

      /**
       * Incomplete type specification for AntennaMaster lets us
       * declare it as a friend below without defining it
       */
      class AntennaMaster;
      
      /**
       * AntennaRoach class will handle all receiver functions. 
       */
      class AntennaRoach : 
	public SpecificTask,
	public gcp::util::GenericTask<AntennaRoachMsg> {
	
	public:
	
	friend class AntennaMaster;

	/**
	 * A pointer to the parent task resources
	 */
	AntennaMaster* parent_;
	
	static AntennaRoach* antennaRoach_;

	/**
	 * Constructor. 
	 */
	AntennaRoach(AntennaMaster* parent);
	AntennaRoach(AntennaMaster* parent, bool simRoach);
	
	/**
	 * Destructor.
	 */
	~AntennaRoach();


	/** 
	 *  Backend objects
	 */
	RoachBackend* roach1_;
	RoachBackend* roach2_;

	bool sim_;

	private:

	/**
	 * Process a message received on the AntennaRoach message queue.
	 */
	void processMsg(AntennaRoachMsg* msg);

	/**
	 * Send Message of connectedness to master thread
	 */
	void sendRoachConnectedMsg(bool connected);
	void sendRoachTimerMsg(bool timeOn);

	void executeRoachCmd(AntennaRoachMsg* msg);

      }; // End class AntennaRoach
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef GCP_ANTENNA_CONTROL_ANTENNARX_H
