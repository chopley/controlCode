// $Id: Agilent33220AWaveformGenerator.h,v 1.1 2009/08/17 17:20:55 eml Exp $

#ifndef GCP_UTIL_AGILENT33220AWAVEFORMGENERATOR_H
#define GCP_UTIL_AGILENT33220AWAVEFORMGENERATOR_H

/**
 * @file Agilent33220AWaveformGenerator.h
 * 
 * Tagged: Wed Aug 12 13:41:47 PDT 2009
 * 
 * @version: $Revision: 1.1 $, $Date: 2009/08/17 17:20:55 $
 * 
 * @author tcsh: username: Command not found.
 */
#include "gcp/util/common/GpibUsbDevice.h"

namespace gcp {
  namespace util {

    class Agilent33220AWaveformGenerator : public GpibUsbDevice {
    public:

      /**
       * Constructor.
       */
      Agilent33220AWaveformGenerator();
      Agilent33220AWaveformGenerator(GpibUsbController& controller);
      
      /**
       * Destructor.
       */
      virtual ~Agilent33220AWaveformGenerator();

      enum OutputType {
	SINUSOID,
	SQUARE,
	RAMP, 
	PULSE,
	NOISE,
	DC,
	USER
      };

      void setOutputType(OutputType type);

      std::string getLastError();

    private:
    }; // End class Agilent33220AWaveformGenerator

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_AGILENT33220AWAVEFORMGENERATOR_H
