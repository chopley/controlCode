// $Id: GpibUsbDevice.h,v 1.1 2009/08/13 21:34:51 eml Exp $

#ifndef GCP_UTIL_GPIBUSBDEVICE_H
#define GCP_UTIL_GPIBUSBDEVICE_H

/**
 * @file GpibUsbDevice.h
 * 
 * Tagged: Tue Aug 11 14:04:19 PDT 2009
 * 
 * @version: $Revision: 1.1 $, $Date: 2009/08/13 21:34:51 $
 * 
 * @author tcsh: username: Command not found.
 */
#include "gcp/util/common/GpibUsbController.h"

#include <string>

namespace gcp {
  namespace util {

    class GpibUsbDevice {
    public:

      // Constructor.

      GpibUsbDevice(bool doSpawn=false);
      GpibUsbDevice(std::string port, bool doSpawn=false);
      GpibUsbDevice(GpibUsbController& controller);

      // Destructor.

      virtual ~GpibUsbDevice();

      // Set the address for this device

      void setAddress(unsigned address);
      unsigned  getAddress();

      // Convenience accessor methods to the controller

      void sendDeviceCommand(std::string cmd, bool expectsResponse=false, 
			     GPIB_RESPONSE_HANDLER(*handler)=0, bool block=false, 
			     void* retVal=0);

      void sendControllerCommand(std::string cmd, bool expectsResponse=false, 
				 GPIB_RESPONSE_HANDLER(*handler)=0, bool block=false, void* retVal=0);

      std::string getDevice();

      GpibUsbController* controller() {
	return controller_;
      }

    private:

      unsigned address_;
      GpibUsbController* controller_;
      bool ownController_;

    }; // End class GpibUsbDevice

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_GPIBUSBDEVICE_H
