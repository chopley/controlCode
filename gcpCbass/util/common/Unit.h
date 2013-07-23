// $Id: Unit.h,v 1.1.1.1 2009/07/06 23:57:27 eml Exp $

#ifndef GCP_UTIL_UNIT_H
#define GCP_UTIL_UNIT_H

/**
 * @file Unit.h
 * 
 * Tagged: Sun Oct 22 19:22:27 PDT 2006
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:27 $
 * 
 * @author username: Command not found.
 */
#include <string>
#include <vector>

namespace gcp {
  namespace util {

    // This class is intended as a base-class for all unit classes
    // defined in inheritors of ConformableQuantity class

    class Unit {
    public:

      /**
       * Constructor.
       */
      Unit();

      /**
       * Destructor.
       */
      virtual ~Unit();

      // Return true if the passed name is a recognized name for this
      // unit

      bool isThisUnit(std::string unitName);

    protected:

      // A collection of valid name for this unit

      std::vector<std::string> names_;
	
      // Add a name to this list

      void addName(std::string name);

      virtual void addNames();

    }; // End class Unit

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_UNIT_H
