#ifndef GCP_ANTENNA_CONTROL_ANTENNARX_H
#define GCP_ANTENNA_CONTROL_ANTENNARX_H

/**
 * @file AntennaRx.h
 * 
 * Tagged: Mon Nov 23 15:59:36 PST 2009
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/GenericTask.h"

#include "gcp/antenna/control/specific/AntennaRxMsg.h"
#include "gcp/antenna/control/specific/Backend.h"
#include "gcp/antenna/control/specific/SpecificTask.h"
#include "gcp/util/specific/Directives.h"

namespace gcp {
  namespace antenna {

    namespace control {

      /**
       * Incomplete type specification for AntennaMaster lets us
       * declare it as a friend below without defining it
       */
      class AntennaMaster;
      
      /**
       * AntennaRx class will handle all receiver functions. 
       */
      class AntennaRx : 
	public SpecificTask,
	public gcp::util::GenericTask<AntennaRxMsg> {
	
	public:
	
	friend class AntennaMaster;

	/**
	 * A pointer to the parent task resources
	 */
	AntennaMaster* parent_;
	
	static AntennaRx* antennaRx_;

	/**
	 * Constructor. 
	 */
	AntennaRx(AntennaMaster* parent);
	
	/**
	 * Destructor.
	 */
	~AntennaRx();


	/** 
	 *  Backend object
	 */
	Backend* backend_;

	private:

	/**
	 * Process a message received on the AntennaRx message queue.
	 */
	void processMsg(AntennaRxMsg* msg);

	/**
	 * Send Message of connectedness to master thread
	 */
	void sendRxConnectedMsg(bool connected);
	void sendRxTimerMsg(bool timeOn);

	void executeRxCmd(AntennaRxMsg* msg);

	void getBurstData();
	
      }; // End class AntennaRx
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef GCP_ANTENNA_CONTROL_ANTENNARX_H
