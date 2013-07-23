#ifndef GCP_UTIL_ANTENNANETCMDFORWARDER_H
#define GCP_UTIL_ANTENNANETCMDFORWARDER_H

/**
 * @file AntennaNetCmdForwarder.h
 * 
 * Tagged: Sun May 16 12:27:04 PDT 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/NetCmdForwarder.h"

namespace gcp {
  namespace util {
    
    class AntennaNetCmdForwarder : public NetCmdForwarder {
    public:
      
      /**
       * Constructor.
       */
      AntennaNetCmdForwarder();
      
      /**
       * Destructor.
       */
      virtual ~AntennaNetCmdForwarder();
      
      //------------------------------------------------------------
      // Overwrite the base-class method by which all rtc commands for
      // the antennas are processed
      //------------------------------------------------------------
      
      /**
       * A virtual method to forward a command received from the ACC.
       * Make this virtual so that inheritors can completely redefine
       * what happens with a received command, if they wish.
       */
      virtual void forwardNetCmd(gcp::util::NetCmd* netCmd);
      
    protected:
      
      /**
       * Forward a control command 
       */
      virtual void forwardControlNetCmd(gcp::util::NetCmd* netCmd);
      
      /**
       * Optical Camera commands
       */
      virtual void forwardOpticalCameraNetCmd(gcp::util::NetCmd* netCmd);
           
      /**
       * RxSim commands
       */
      virtual void forwardRxSimulatorNetCmd(gcp::util::NetCmd* netCmd);
            
      /**
       * Forward a command for the weather station
       */
      virtual void forwardWeatherNetCmd(gcp::util::NetCmd* netCmd) ;
      
      /**
       * Forward a tracker net command
       */
      virtual void forwardTrackerNetCmd(gcp::util::NetCmd* netCmd);
      
      /**
       * Scanner task commands
       */
      virtual void forwardScannerNetCmd(gcp::util::NetCmd* netCmd);
         
      /**
       * AntennaRx task commands
       */
      virtual void forwardRxNetCmd(gcp::util::NetCmd* netCmd);

      /**
       * AntennaRoach task commands
       */
      virtual void forwardRoachNetCmd(gcp::util::NetCmd* netCmd);

      /**
       * AntennaLna task commands
       */
      virtual void forwardLnaNetCmd(gcp::util::NetCmd* netCmd);

      /**
       * Board flagging commands
       */
      virtual void forwardBoardNetCmd(gcp::util::NetCmd* netCmd);
      
    }; // End class AntennaNetCmdForwarder
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_ANTENNANETCMDFORWARDER_H
