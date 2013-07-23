// $Id: Test.h,v 1.1.1.1 2009/07/06 23:57:24 eml Exp $

#ifndef GCP_UTIL_TEST.H_H
#define GCP_UTIL_TEST.H_H

/**
 * @file Test.h.h
 * 
 * Tagged: Fri May 18 11:37:04 PDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:24 $
 * 
 * @author username: Command not found.
 */

#include <iostream>

namespace gcp {
  namespace util {

    class Test.h {
    public:

      /**
       * Constructor.
       */
      Test.h();

      /**
       * Copy Constructor.
       */
      Test.h(const Test.h& objToBeCopied);

      /**
       * Copy Constructor.
       */
      Test.h(Test.h& objToBeCopied);

      /**
       * Const Assignment Operator.
       */
      void operator=(const Test.h& objToBeAssigned);

      /**
       * Assignment Operator.
       */
      void operator=(Test.h& objToBeAssigned);

      /**
       * Output Operator.
       */
      friend std::ostream& operator<<(std::ostream& os, Test.h& obj);

      /**
       * Destructor.
       */
      virtual ~Test.h();

    private:
    }; // End class Test.h

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_TEST.H_H
