#ifndef GCP_ANTENNA_CONTROL_ANTNETCMDFORWARDER_H
#define GCP_ANTENNA_CONTROL_ANTNETCMDFORWARDER_H

/**
 * @file AntNetCmdForwarder.h
 * 
 * Tagged: Wed Mar 17 19:23:17 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"
#include "gcp/util/specific/AntennaNetCmdForwarder.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      class AntennaMaster;

      class AntNetCmdForwarder : public gcp::util::AntennaNetCmdForwarder {
      public:
	
	/**
	 * Constructor.
	 */
	AntNetCmdForwarder(AntennaMaster* parent);
	
	/**
	 * Destructor.
	 */
	virtual ~AntNetCmdForwarder();

    private:

      /**
       * The parent resources.
       */
	AntennaMaster* parent_;

      //------------------------------------------------------------
      // Methods by which individual rtc commands are forwarded
      //------------------------------------------------------------

      /**
       * Forward a Tracker command.
       */
      void forwardTrackerNetCmd(gcp::util::NetCmd* netCmd);

      /**
       * Forward a control command 
       */
      void forwardControlNetCmd(gcp::util::NetCmd* netCmd);

      /**
       * Forward an antenna rx command 
       */
      void forwardRxNetCmd(gcp::util::NetCmd* netCmd);

      /**
       * Forward an antenna roach command 
       */
      void forwardRoachNetCmd(gcp::util::NetCmd* netCmd);

      /**
       * Forward an antenna lna command 
       */
      void forwardLnaNetCmd(gcp::util::NetCmd* netCmd);
      
      }; // End class AntNetCmdForwarder
      
    } // End namespace control
  } // End namespace antenna
} // End namespace gcp



#endif // End #ifndef GCP_ANTENNA_CONTROL_ANTNETCMDFORWARDER_H
