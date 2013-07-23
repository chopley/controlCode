// $Id: DliPowerStripController.h,v 1.1 2010/11/24 00:45:25 sjcm Exp $

#ifndef GCP_UTIL_DLIPOWERSTRIPCONTROLLER_H
#define GCP_UTIL_DLIPOWERSTRIPCONTROLLER_H

/**
 * @file DliPowerStripController.h
 * 
 * Tagged: xrdb: No such file or directory
 * 
 * @version: $Revision: 1.1 $, $Date: 2010/11/24 00:45:25 $
 * 
 * @author xrdb: No such file or directory
 */

#include <iostream>

#include "gcp/util/common/DliPowerStrip.h"
#include "gcp/util/common/GenericTaskMsg.h"
#include "gcp/util/common/SpawnableTask.h"

namespace gcp {
  namespace util {

    // A utility class for sending messages to the GpibUsbController task
      
    class DliPowerStripControllerMsg : public GenericTaskMsg {
    public:
      
      enum MsgType {
	ENABLE,
	CYCLE,
	STATUS,
      };

      union {
	struct {
	  bool enable_;
	  DliPowerStrip::Outlet mask_;
	} enable;

	struct {
	  DliPowerStrip::Outlet mask_;
	} cycle;

      } body;

      // A type for this message
      
      MsgType type_;
    };
      
    class DliPowerStripController : public SpawnableTask<DliPowerStripControllerMsg> {
    public:

      /**
       * Constructor.
       */
      DliPowerStripController(std::string host, bool doSpawn=true);

      /**
       * Output Operator.
       */
      friend std::ostream& operator<<(std::ostream& os, DliPowerStripController& obj);

      /**
       * Destructor.
       */
      virtual ~DliPowerStripController();

      void on(DliPowerStrip::Outlet mask);
      void off(DliPowerStrip::Outlet mask);
      void cycle(DliPowerStrip::Outlet mask);
      void status();

    protected:

      virtual void reportStatus(std::vector<DliPowerStrip::State>& status) {};

      /**.......................................................................
       * Main Task event loop: when this is called, the task blocks forever
       * in select(), or until a stop message is received.
       */
      void processMsg(DliPowerStripControllerMsg* msg);
	
    private:

      DliPowerStrip dli_;

    }; // End class DliPowerStripController

  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_DLIPOWERSTRIPCONTROLLER_H
