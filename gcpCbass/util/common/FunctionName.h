// $Id: FunctionName.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_FUNCTIONNAME_H
#define GCP_UTIL_FUNCTIONNAME_H

/**
 * @file FunctionName.h
 * 
 * Tagged: Sat Jan 20 22:15:41 NZDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author Erik Leitch
 */

#include <iostream>

namespace gcp {
  namespace util {

    class FunctionName {
    public:

      /**
       * Constructor.
       */
      FunctionName(const char* prettyFunction);

      /**
       * Destructor.
       */
      virtual ~FunctionName();

      std::string prettyFunction();
      std::string noArgs();

      static std::string prettyFunction(const char* prettyFunction);
      static std::string noArgs(const char* prettyFunction);

    private:

      std::string prettyFunction_;

    }; // End class FunctionName

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_FUNCTIONNAME_H
