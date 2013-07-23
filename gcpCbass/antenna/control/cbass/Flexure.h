#ifndef GCP_ANTENNA_CONTROL_FLEXURE_H
#define GCP_ANTENNA_CONTROL_FLEXURE_H

/**
 * @file Flexure.h
 * 
 * Tagged: Wed Dec 15 14:16:24 CST 2004
 * 
 * @author Erik Leitch
 */
#include <iostream>

namespace gcp {
  namespace antenna {
    namespace control {
      
      class PointingCorrections;

      class Flexure {
      public:
	
	/**
	 * Constructor.
	 */
	Flexure();
	
	/**
	 * Destructor.
	 */
	virtual ~Flexure();

	void setSineElFlexure(double sFlexure);
	void setCosElFlexure(double cFlexure);
	
	void apply(PointingCorrections* f);

	void setUsable(bool usable);
	bool isUsable();

	void reset();

	void pack(signed* s_elements);

	friend std::ostream& operator<<(std::ostream& os, Flexure& flex);;

      private:

	bool usable_;

	// The coefficient of the sin(el) term

	double sFlexure_;

	// The coefficient of the cos(el) term

	double cFlexure_;

      }; // End class Flexure
      
      std::ostream& operator<<(std::ostream& os, Flexure& flex);

    } // End namespace control
  } // End namespace antenna
} // End namespace gcp



#endif // End #ifndef GCP_ANTENNA_CONTROL_FLEXURE_H
