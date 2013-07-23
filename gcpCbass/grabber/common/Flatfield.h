#ifndef GCP_GRABBER_FLATFIELD_H
#define GCP_GRABBER_FLATFIELD_H

/**
 * @file Flatfield.h
 * 
 * Tagged: Tue 24-Jan-06 07:48:32
 * 
 * @author Erik Leitch
 */
namespace gcp {
  namespace grabber {
    
    class Flatfield {
    public:
      
      enum {
	FLATFIELD_NONE,
	FLATFIELD_ROW,
	FLATFIELD_IMAGE
      };

    }; // End class Flatfield
    
  } // End namespace grabber
} // End namespace gcp



#endif // End #ifndef GCP_GRABBER_FLATFIELD_H
