// $Id: Sort.h,v 1.1.1.1 2009/07/06 23:57:26 eml Exp $

#ifndef GCP_UTIL_SORT_H
#define GCP_UTIL_SORT_H

/**
 * @file Sort.h
 * 
 * Tagged: Wed Feb 14 09:35:58 NZDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:26 $
 * 
 * @author Erik Leitch
 */

#include <iostream>
#include <vector>
#include <string>

namespace gcp {
  namespace util {

    class Sort {
    public:

      /**
       * Constructor.
       */
      Sort();

      /**
       * Destructor.
       */
      virtual ~Sort();

      static std::vector<std::string> sort(std::vector<std::string>& entries);

    }; // End class Sort

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_SORT_H
