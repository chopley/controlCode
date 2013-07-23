#ifndef OPTICALBENCH_H
#define OPTICALBENCH_H

/**
 * @file OpticalBench.h
 * 
 * Tagged: Thu Nov 13 16:53:55 UTC 2003
 * 
 * @author Ken Aird
 */

#include "gcp/util/common/Angle.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      /**
       * Define a class that will handle pointing and tracking for
       * this antenna.
       */
      class OpticalBench {
	
	public:
	
	/**
         * Constructor
	 */
	OpticalBench();
	
	/**
	 * Destructor.
	 */
	~OpticalBench();
	
        void setZeroPosition(double y1, double y2, double y3, double x4, double x5, double z6);	
        void setOffset(double y1, double y2, double y3, double x4, double x5, double z6);	
        void setUseBrakes(bool use_brakes);
        void setAcquiredThreshold(double threshold);
        double getAcquiredThreshold() {return acquiredThreshold_;};
        void setFocus(double focus);
        double getDeadBand();
        double getFocus() {return focus_;};;
        double enforceY1Limits(double y1);
        double enforceY2Limits(double y2);
        double enforceY3Limits(double y3);
        double enforceX4Limits(double x4);
        double enforceX5Limits(double x5);
        double enforceZ6Limits(double z6);
        double getExpectedY1();
        double getExpectedY2();
        double getExpectedY3();
        double getExpectedX4();
        double getExpectedX5();
        double getExpectedZ6();

        double getZerosY1() {return zeroPosition_.y1;};
        double getZerosY2() {return zeroPosition_.y2;};
        double getZerosY3() {return zeroPosition_.y3;};
        double getZerosX4() {return zeroPosition_.x4;};
        double getZerosX5() {return zeroPosition_.x5;};
        double getZerosZ6() {return zeroPosition_.z6;};

        double getOffsetY1() {return offset_.y1;};
        double getOffsetY2() {return offset_.y2;};
        double getOffsetY3() {return offset_.y3;};
        double getOffsetX4() {return offset_.x4;};
        double getOffsetX5() {return offset_.x5;};
        double getOffsetZ6() {return offset_.z6;};

        private:
         
        class OpticalBenchPosition {
        public:
          double y1; 
          double y2; 
          double y3; 
          double x4; 
          double x5; 
          double z6; 
        };

        class OpticalBenchOffset {
        public:
          double y1; 
          double y2; 
          double y3; 
          double x4; 
          double x5; 
          double z6; 
        };

        OpticalBenchPosition zeroPosition_;
        OpticalBenchPosition lowLimit_;
        OpticalBenchPosition highLimit_;
        OpticalBenchOffset   offset_;
        bool                 useBrakes_;
        double               acquiredThreshold_;
        double               focus_;
        gcp::util::Angle     theta_;
	
      }; // End class Tracker
      
    }; // End namespace control
  }; // End namespace antenna
}; // End namespace gcp

#endif
