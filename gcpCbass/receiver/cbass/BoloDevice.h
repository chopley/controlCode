// $Id: BoloDevice.h,v 1.1.1.1 2009/07/06 23:57:23 eml Exp $

#ifndef GCP_RECEIVER_BOLODEVICE_H
#define GCP_RECEIVER_BOLODEVICE_H

/**
 * @file BoloDevice.h
 * 
 * Tagged: Sat Dec 16 14:12:47 PST 2006
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:23 $
 * 
 * @author Erik Leitch
 */
#include <string>

namespace gcp {
  namespace receiver {

    class BoloDevice {
    public:

      /**
       * Constructor.
       */
      BoloDevice();

      /**
       * Copy Constructor.
       */
      BoloDevice(const BoloDevice& objToBeCopied);

      /**
       * Copy Constructor.
       */
      BoloDevice(BoloDevice& objToBeCopied);

      /**
       * Const Assignment Operator.
       */
      void operator=(const BoloDevice& objToBeAssigned);

      /**
       * Assignment Operator.
       */
      void operator=(BoloDevice& objToBeAssigned);

      /**
       * Output Operator.
       */
      friend std::ostream& operator<<(std::ostream& os, BoloDevice& obj);

      /**
       * Destructor.
       */
      virtual ~BoloDevice();

      void setId(std::string id);
      void setReadoutChannel(std::string channel);

      std::string getId();
      std::string getReadoutChannel();

    private:

      std::string id_;             // The string id of this channel
      std::string readoutChannel_; // The readout channel associated
				   // with this device
    }; // End class BoloDevice

  } // End namespace receiver
} // End namespace gcp


#endif // End #ifndef GCP_RECEIVER_BOLODEVICE_H
