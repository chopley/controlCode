#ifndef ENCODER_H
#define ENCODER_H

/**
 * @file Encoder.h
 * 
 * Tagged: Thu Nov 13 16:53:37 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Axis.h"
#include "gcp/util/common/Angle.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      class PmacAxis;
      
      /**
       * Objects of the following type are used to aggregate the
       * encoder calibration parameters of each axis.
       */
      class Encoder {
	
      public:
	
	/**
	 * Constructor
	 */
	Encoder(gcp::util::Axis::Type axis);
	
	/**
	 * Reset the parameters of this object
	 */
	void reset();
	
	/**
	 * Set the zero point of this encoder.
	 */
	void setZero(double zero);
	
	/**
	 * Set the slew rate.
	 */
	void setSlewRate(long rate);
	
	/**
	 * Set the counts per turn for this encoder.
	 */
	void setCountsPerTurn(int per_turn);
	
	/** 
	 * Set the counts per radian for this encoder.
	 */
	void setCountsPerRadian(double countsPerRadian);
	
	/**
	 * Set the limits.
	 */
	void setLimits(long min, long max);
	void setLimits(gcp::util::Angle& min, gcp::util::Angle& max);
	
	/**
	 * Return the axis type of this encoder
	 */
	gcp::util::Axis::Type getAxis();
	
	/**
	 * Convert from radians to encoder values on a given axis
	 */
	void convertMountToEncoder(double angle, double rate, 
				   PmacAxis* axis, int current);

	/**
	 * Applies the limits to the axis values
	 */
	void applyLimits(double angle, double rate, 
			 PmacAxis* axis, double current);
	/**
	 * Update the mount limits.
	 */
	void updateMountLimits();
	
	/**
	 * Get the mount minimum.
	 */
	double getMountMin();
	
	/**
	 * Get the mount maximum.
	 */
	double getMountMax();
	
	/**
	 * Get the slew rate.
	 */
	signed getSlewRate();
	
	/**
	 * Convert from encoder counts to radians on the sky
	 */
	double convertCountsToSky(int count);
	
	double convertAngleToSky(gcp::util::Angle& angle);

        gcp::util::Angle convertSkyToAngle(gcp::util::Angle sky);
	
	/**
	 * Pack encoder zero points for archival in the register
	 * database.
	 */
	void packZero(signed* s_elements);
	
	/**
	 * Pack this encoder multiplier for archival in the register
	 * database.
	 */
	void packCountsPerTurn(signed* s_elements);
	
	/**
	 * Method for packing data to be archived in the register
	 * database
	 */
	void packLimits(signed* s_elements);
	
      private:
	
	/**
	 * Which axis does this encoder represent?
	 */
	gcp::util::Axis::Type axis_;       
	
	/**
	 * Encoder count for an angle of zero
	 */
	double zero_;           
	
	/**
	 * Encoder counts per radian (can be negative)
	 */
	double countsPerRadian_;
	
	/**
	 * Encoder counts per turn (always positive)
	 */
	int countsPerTurn_;     
	
	/**
	 * The minimum legal encoder count
	 */
	int min_;               
	
	/**
	 * The maximum legal encoder count
	 */
	int max_;               

	/*
	 * The minimum angle limit
	 */
	gcp::util::Angle minLim_;               
	
	/**
	 * The maximum angle limit
	 */
	gcp::util::Angle maxLim_;               
	
	/**
	 * The mount angle corresponding to min
	 */
	double mountMin_;       
	
	/**
	 * The mount angle corresponding to max
	 */
	double mountMax_;       
	
	/**
	 * The slew rate of this axis (radians)
	 */
	signed slewRate_;       
	
      }; // End class Encoder
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
