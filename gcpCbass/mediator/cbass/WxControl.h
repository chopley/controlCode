#ifndef GCP_ASSEMBLER_WXCONTROL_H
#define GCP_ASSEMBLER_WXCONTROL_H

/**
 * @file WxControl.h
 * 
 * Tagged: Mon Jul 19 11:34:40 PDT 2004
 * 
 * @author Erik Leitch
 */
#include <string>
#include <list>

#include "gcp/mediator/specific/WxControlMsg.h"

#include "gcp/util/common/Angle.h"
#include "gcp/util/common/Date.h"
#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/Percent.h"
#include "gcp/util/common/Pressure.h"
#include "gcp/util/common/Speed.h"
#include "gcp/util/common/Temperature.h"
#include "gcp/util/common/TcpClient.h"
#include "gcp/util/common/TimeVal.h"

namespace gcp {
  
  namespace util {
    class MonitorPoint;
    class MonitorPointManager;
  }
  
namespace mediator {
    
    /**
     * Incomplete type specification for Control lets us
     * declare it as a friend below without defining it
     */
    class Control;
    class WxClientCbass;
    
    /**
     * A class for managing communications with the weather station
     */
    class WxControl :
      public gcp::util::GenericTask<WxControlMsg> {
      
      public:
      

      struct WxData {
	gcp::util::Temperature airTemperature_;
	gcp::util::Speed       windSpeed_;
	gcp::util::Angle       windDirection_;
	gcp::util::Pressure    pressure_;
	gcp::util::Percent     relativeHumidity_;
	gcp::util::Date        sampleTime_;
      };

      /**
       * Constructor.
       */
      WxControl(Control* parent);
      
      /**
       * Destructor.
       */
      virtual ~WxControl();
      
      /**
       * Decrement/Increment the count of pending communications
       */
      void incrementPending();
      void decrementPending();
      
      void writeVals(WxData& data);
      
      private:
      
      friend class Control;
      friend class SptWxClient;
      
      Control* parent_;
      
      unsigned pending_;
      
      gcp::util::TimeVal timer_;
      struct timeval* timeOut_;
      
      /**
       * An object for communicating with the weather stations via TCP/IP
       */
      WxClientCbass* wx_;
      
      gcp::util::MonitorPointManager* monitor_;
 
      gcp::util::MonitorPoint* monUtc_;
      gcp::util::MonitorPoint* monAirTemp_;
      gcp::util::MonitorPoint* monRelHum_;
      gcp::util::MonitorPoint* monWindSpeed_;
      gcp::util::MonitorPoint* monWindDir_;
      gcp::util::MonitorPoint* monPressure_;
      
      WxData data_;

      /**
       * Set a timeout for waiting for a response from the weather station.
       */
      void enableTimeOut(bool enable);
      
      /**
       * Initiate communciations with one or more power strips
       */
      void initiateReadValuesCommSequence();
      
      /**
       * Service our message queue.
       */
      void serviceMsgQ();
      
      /**
       * Override the base-class method to process messages for this
       * task
       */
      void processMsg(WxControlMsg* msg);

      /**
       * Send pertinent data to the antennas for refraction
       * calculations
       */
      void distributeWeatherData(WxData& data);
      
    }; // End class WxControl
    
} // End namespace mediator
} // End namespace gcp



#endif // End #ifndef GCP_ASSEMBLER_WXCONTROL_H
