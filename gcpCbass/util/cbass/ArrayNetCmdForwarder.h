#ifndef GCP_UTIL_ARRAYNETCMDFORWARDER_H
#define GCP_UTIL_ARRAYNETCMDFORWARDER_H

/**
 * @file ArrayNetCmdForwarder.h
 * 
 * Tagged: Wed Mar 17 19:02:51 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/NetCmdForwarder.h"

#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"
#include "gcp/util/common/NetCmd.h"

namespace gcp {
  namespace util {
    
    /**
     * Master class for forwarding message intended for different array
     * subsystems
     */
    class ArrayNetCmdForwarder : public NetCmdForwarder {
    public:
      
      /**
       * Constructor.
       */
      ArrayNetCmdForwarder();
      
      /**
       * Destructor.
       */
      virtual ~ArrayNetCmdForwarder();
      
      //------------------------------------------------------------
      // Overwrite the base-class method by which all rtc commands are
      // processed
      //------------------------------------------------------------
      
      /**
       * A virtual method to forward a command received from the ACC.
       * Make this virtual so that inheritors can completely redefine
       * what happens with a received command, if they wish.
       */
      virtual void forwardNetCmd(gcp::util::NetCmd* netCmd);
      
    protected:
      
      //------------------------------------------------------------
      // Objects which will handle message forwarding for each
      // subsystem.  We make these pointers so that they can point to
      // inheritors of subsystem base classes if we wish.  For example,
      // we might have two different types of antenna command
      // forwarders, one for CORBA communications, and one for TCP/IP
      //------------------------------------------------------------
      
      /**
       * Antenna subsystem
       */
      NetCmdForwarder* antennaForwarder_;
      
      /** 
       * Control command intended for the translator itself
       */
      NetCmdForwarder* controlForwarder_;
      
      /** 
       * Downconverter subsystem
       */
      NetCmdForwarder* dcForwarder_;
      
      /** 
       * Delay subsystem
       */
      NetCmdForwarder* delayForwarder_;
      
      /** 
       * Receiver control subsystem
       */
      NetCmdForwarder* receiverForwarder_;
      
      /** 
       * Frame Grabber subsystem
       */
      NetCmdForwarder* grabberForwarder_;
      
      /** 
       * Pointing telescope subsystem
       */
      NetCmdForwarder* ptelForwarder_;

      /** 
       * Deicing subsystem
       */
      NetCmdForwarder* deicingForwarder_;

      /** 
       * Scanner command intended for the mediator itself
       */
      NetCmdForwarder* scannerForwarder_;
      
      /** 
       * Scanner command intended for the antenna power strips
       */
      NetCmdForwarder* stripForwarder_;
      
    private:
      
      /**
       * Forward a command to the antenna subsystem
       */
      void forwardAntennaNetCmd(gcp::util::NetCmd* netCmd);
      
      /**
       * Forward a control command
       */
      void forwardControlNetCmd(gcp::util::NetCmd* netCmd);
      
      /**
       * Forward a command to the dc subsystem
       */
      void forwardDcNetCmd(gcp::util::NetCmd* netCmd);
      
      /**
       * Forward a command to the delay subsystem
       */
      void forwardDelayNetCmd(gcp::util::NetCmd* netCmd);
      
      /**
       * Forward a command to the frame grabber subsystem
       */
      void forwardGrabberNetCmd(gcp::util::NetCmd* netCmd);
      
      /**
       * Forward a command to the receiver subsystem
       */
      void forwardReceiverNetCmd(gcp::util::NetCmd* netCmd);

      /**
       * Forward a command to the pointing telescope
       */
      void forwardPtelNetCmd(gcp::util::NetCmd* netCmd);
      
      /**
       * Forward a command to the deicing heater controller
       */
      void forwardDeicingNetCmd(gcp::util::NetCmd* netCmd);
      
      /**
       * Forward a scanner command
       */
      void forwardScannerNetCmd(gcp::util::NetCmd* netCmd);
      
      /**
       * Forward a strip command
       */
      void forwardStripNetCmd(gcp::util::NetCmd* netCmd);
      
    }; // End class ArrayNetCmdForwarder
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_ARRAYNETCMDFORWARDER_H
