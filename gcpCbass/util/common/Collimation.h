// $Id: Collimation.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_COLLIMATION_H
#define GCP_UTIL_COLLIMATION_H

/**
 * @file Collimation.h
 * 
 * Tagged: Thu Aug 18 18:59:26 PDT 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author BICEP Software Development
 */
namespace gcp {
  namespace util {

    class Collimation {
    public:

      enum Type {
	FIXED = 0x1,
	POLAR = 0x2
      };

      /**
       * Constructor.
       */
      Collimation();

      /**
       * Destructor.
       */
      virtual ~Collimation();

    private:
    }; // End class Collimation

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_COLLIMATION_H
