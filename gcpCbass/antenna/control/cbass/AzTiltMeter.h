#ifndef GCP_UTIL_AZ_TILT_METER_H
#define GCP_UTIL_AZ_TILT_METER_H

#include "gcp/util/common/MovingAverage.h"
#include "gcp/antenna/control/specific/OffsetBase.h"
#include "gcp/util/common/Angle.h"
#include "gcp/util/common/Exception.h"

namespace gcp {
  namespace antenna {
    namespace control {

      using namespace gcp::util;
    
      class AzTiltMeter : public OffsetBase {

      public:

        AzTiltMeter();
        ~AzTiltMeter();
        // Set number of samples in moving average and reset average window to empty
        void setMovingAvgSampleCount(unsigned int count);
        // Set number of samples between tilt updates
        void enable(); // Enable tilt updates
        void disable(); // Disable tilt updates and reset moving average window to empty.
        void reset(); // Reset moving average windows to empty
        // Set angle between Grid North and x tilt meter axis
        void setTheta(Angle theta);
        // Set maximum absolute value to which moving average of each tilt meter reading is clipped
        void setRange(Angle maxAngle) {
          maxAngle_ = maxAngle.radians();
          COUT("AzTiltMeter maxAngle = " << maxAngle.degrees());
        }
        // Set offset to be added to raw tilt meter values to zero tilt meter
        void setOffset(Angle x, Angle y) {
          COUT("AzTiltMeter offset x = " << x.degrees() << " y = " << y.degrees());
          xOffset_ = x.radians(); yOffset_ = y.radians();
        };
        
        void addSample(Angle x, Angle y); // Called for each valid set of tilt meter readings
        // Return hour angle and lattitude tilt corrections for specified azimuth
        void apply(PointingCorrections *f); 
        Angle xAvg() {return Angle(Angle::Radians(), *xAvg_);};
        Angle yAvg() {return Angle(Angle::Radians(), *yAvg_);};
        Angle xOffset() {return Angle(Angle::Radians(), xOffset_);};
        Angle yOffset() {return Angle(Angle::Radians(), yOffset_);};
        bool enabled() {return meterEnabled_;};
        Angle maxAngle() {return Angle(Angle::Radians(), maxAngle_);};
        Angle theta() {return theta_;};
        unsigned int movingAvgSampleCount() {return avgCount_;}; 
        bool lacking(); // True if disabled or moving average window not full

      private:
       
        const static unsigned int defaultAvgCount = 1000; // Default number of samples in moving average
        unsigned int avgCount_; // number of samples in moving average
        MovingAverage<double>* xAvg_; // Moving average of x tilt meter value
        MovingAverage<double>* yAvg_; // Moving average of y tilt meter value
        bool meterEnabled_; // True if tilt correction based on meter enabled
        Angle theta_; // Angle between grid North and x tilt meter axis
                                 // increases counterclockwise looking down
        double maxAngle_; // Max allowed absolute value of tilt meter angle in radians
        double xOffset_;
        double yOffset_;
        // cached trig functions
        double sinTheta_;
        double cosTheta_;
      }; 
             
    } // End namespace control
  } // End namespace antenna 
} // End namespace gcp
      
#endif // End #ifndef GCP_UTIL_AZ_TILT_METER_H
