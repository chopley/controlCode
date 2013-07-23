// $Id: Collimation.h,v 1.1.1.1 2009/07/06 23:57:04 eml Exp $

#ifndef GCP_ANTENNA_CONTROL_COLLIMATION_H
#define GCP_ANTENNA_CONTROL_COLLIMATION_H

/**
 * @file Collimation.h
 * 
 * Tagged: Thu Aug 18 18:40:57 PDT 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:04 $
 * 
 * @author EXPSTUB Software Development
 */
#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"

#include "gcp/antenna/control/specific/PointingCorrections.h"
#include "gcp/antenna/control/specific/TrackerOffset.h"


namespace gcp {
  namespace antenna {
    namespace control {
      class Collimation {
      public:

	/**
	 * Constructor.
	 */
	Collimation();

	/**
	 * Destructor.
	 */
	virtual ~Collimation();

	bool isUsable();
	void setUsable(bool usable);
	virtual void reset();

	virtual void apply(PointingCorrections* f, TrackerOffset& offset) {};

	virtual void pack(signed* s_elements) {};

	virtual void print();

      protected:

	bool usable_;

      }; // End class Collimation
    } // End namespace control
  } // End namespace antenna
} // End namespace gcp

#endif // End #ifndef GCP_ANTENNA_CONTROL_COLLIMATION_H
