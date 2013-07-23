// $Id: SpecificName.h,v 1.2 2009/07/13 22:00:37 eml Exp $

#ifndef GCP_UTIL_SPECIFICNAME_H
#define GCP_UTIL_SPECIFICNAME_H

/**
 * @file SpecificName.h
 * 
 * Tagged: Tue Oct  4 23:53:14 PDT 2005
 * 
 * @version: $Revision: 1.2 $, $Date: 2009/07/13 22:00:37 $
 * 
 * @author Erik Leitch
 */
#include <string>

namespace gcp {
  namespace util {

    class SpecificName {
    public:

      /**
       * Constructor.
       */
      SpecificName();

      /**
       * Destructor.
       */
      virtual ~SpecificName();

      static std::string experimentName();

      static std::string experimentNameCaps();

      static std::string viewerName();

      static std::string controlName();

      static std::string mediatorName();

      static std::string envVarName();

      static bool envVarIsDefined();

    private:

    }; // End class SpecificName

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_SPECIFICNAME_H
