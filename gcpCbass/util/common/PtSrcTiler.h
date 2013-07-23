// $Id: PtSrcTiler.h,v 1.1.1.1 2009/07/06 23:57:26 eml Exp $

#ifndef GCP_UTIL_PTSRCTILER_H
#define GCP_UTIL_PTSRCTILER_H

/**
 * @file PtSrcTiler.h
 * 
 * Tagged: Sat Jun 16 13:51:35 PDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:26 $
 * 
 * @author username: Command not found.
 */
#include "gcp/util/common/Declination.h"
#include "gcp/util/common/HourAngle.h"
#include "gcp/util/common/Angle.h"

#include <vector>

namespace gcp {
  namespace util {

    class PtSrcTiler {
    public:

      // Object to encapsulate a single field to search for point
      // sources

      struct Field {
	HourAngle ra_;     // The center RA of this field
	Declination dec_;  // The center DEC of this field 
	std::vector<Field*> neighbors_; // The 6 fields that
					// surround this one, order
					// in order if increasing clockwise angle
	Angle radius_;     // The radius of this field

	// Constructors

	Field(HourAngle ra, Declination dec, Angle radius);
	Field(const Field& field);
	Field();

	// For the current field, add all nearest neighbors in a hex
	// pattern

	std::vector<Field> addNeighbors(std::vector<Field>& fields, 
					HourAngle& ra0, Declination& dec0,
					Angle& min);

	void addNeighbor(unsigned index, Field* field);
	
	// Return the distance of this field from the given ra/dec
	
	Angle distance(HourAngle ra, Declination dec);

      };

      /**
       * Constructor.
       */
      PtSrcTiler();

      /**
       * Destructor.
       */
      virtual ~PtSrcTiler();

      // Construct the list of fields to search

      static std::vector<Field> constructFields(HourAngle ra,      Declination dec, 
						Angle fieldRadius, Angle totalRadius);


      static void addLayer(unsigned iLayer, Angle& fieldRad, HourAngle& ra0, Declination& dec0, 
			   std::vector<Field>& fields);

    private:
    }; // End class PtSrcTiler

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_PTSRCTILER_H
