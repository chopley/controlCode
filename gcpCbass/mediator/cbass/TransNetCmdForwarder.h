#ifndef GCP_ASSEMBLER_TRANSNETCMDFORWARDER_H
#define GCP_ASSEMBLER_TRANSNETCMDFORWARDER_H

/**
 * @file TransNetCmdForwarder.h
 * 
 * Tagged: Wed Mar 17 19:04:44 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/specific/ArrayNetCmdForwarder.h"

namespace gcp {
namespace mediator {
    
    class Control;
    class ControlMsg;

    class TransNetCmdForwarder : public gcp::util::ArrayNetCmdForwarder {
    public:
      
      /**
       * Constructor.
       */
      TransNetCmdForwarder(Control* parent);
      
      /**
       * Destructor.
       */
      virtual ~TransNetCmdForwarder();
      
      //------------------------------------------------------------
      // Top-level method by which all rtc commands are processed 
      //------------------------------------------------------------
      
      /**
       * A virtual method to forward a command received from the ACC.
       * Make this virtual so that inheritors can completely redefine
       * what happens with a received command, if they wish.
       */
      void forwardNetCmd(gcp::util::NetCmd* netCmd);

    private:

      /**
       * The parent resources.
       */
      Control* parent_;

      /**
       * True if we are currently receiving an initialization script
       */
      bool receivingInitScript_;

      //------------------------------------------------------------
      // Methods by which individual rtc commands are forwarded
      //------------------------------------------------------------

      /**
       * Antenna commands
       */
      void forwardAntennaNetCmd(gcp::util::NetCmd* netCmd);

      /**
       * Forward a control command.
       */
      void forwardControlNetCmd(gcp::util::NetCmd* netCmd);
      
      /**
       * Forward a Tracker command.
       */
      void forwardTrackerNetCmd(gcp::util::NetCmd* netCmd);
      
      /**
       * Forward a Rx command.
       */
      void forwardRxNetCmd(gcp::util::NetCmd* netCmd);

      //------------------------------------------------------------
      // Methods by which commands converted into messages are
      // forwarded to individual tasks.
      //------------------------------------------------------------

      /**
       * Forward a message to the AntennaControl task.
       */
      void forwardAntennaControlMsg(ControlMsg* msg);

      /**
       * Forward a message to the control task.
       */
      void forwardControlMsg(ControlMsg* msg);

      /**
       * Forward a message to the frame grabber control task.
       */
      void forwardGrabberControlMsg(ControlMsg* msg);

      /**
       * Forward a message to the frame ptel control task.
       */
      void forwardPtelControlMsg(ControlMsg* msg);

    }; // End class TranNetCmdForwarder
    
} // End namespace mediator
} // End namespace gcp



#endif // End #ifndef GCP_ASSEMBLER_TRANSNETCMDFORWARDER_H
