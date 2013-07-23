// $Id: BoloPixelManager.h,v 1.1.1.1 2009/07/06 23:57:23 eml Exp $

#ifndef GCP_RECEIVER_BOLOPIXELMANAGER_H
#define GCP_RECEIVER_BOLOPIXELMANAGER_H

/**
 * @file BoloPixelManager.h
 * 
 * Tagged: Sat Dec 16 14:16:06 PST 2006
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:23 $
 * 
 * @author Erik Leitch
 */
#include <string>
#include <vector>

#include "gcp/receiver/specific/BoloPixel.h"

#include "Utilities/HardwareManagerClient.h"

namespace gcp {
  namespace receiver {

    // A class for managing the pixels associated with the current
    // receiver

    class BoloPixelManager {
    public:

      /**
       * Constructors
       */
      BoloPixelManager();
      BoloPixelManager(std::string configFile);
      BoloPixelManager(std::string hwHost, unsigned int hwPort);

      /**
       * Copy Constructor.
       */
      BoloPixelManager(const BoloPixelManager& objToBeCopied);

      /**
       * Copy Constructor.
       */
      BoloPixelManager(BoloPixelManager& objToBeCopied);

      /**
       * Const Assignment Operator.
       */
      void operator=(const BoloPixelManager& objToBeAssigned);

      /**
       * Assignment Operator.
       */
      void operator=(BoloPixelManager& objToBeAssigned);

      /**
       * Destructor.
       */
      virtual ~BoloPixelManager();

      void initialize();
      void initializeFromFile(std::string);
      void initializeFromHardwareManager(std::string hwHost, unsigned int hwPort);

      /**
       * Return a vector of pixels
       */
      std::vector<BoloPixel> getPixels();

    private:

      bool initialized_;
      std::vector<BoloPixel> pixels_;
	
    }; // End class BoloPixelManager

  } // End namespace receiver
} // End namespace gcp


#endif // End #ifndef GCP_RECEIVER_BOLOPIXELMANAGER_H
