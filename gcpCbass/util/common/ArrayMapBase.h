#ifndef GCP_UTIL_ARRAYMAPBASE_H
#define GCP_UTIL_ARRAYMAPBASE_H

/**
 * @file ArrayMapBase.h
 * 
 * Tagged: Wed Oct  6 11:04:54 PDT 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/control/code/unix/libunix_src/common/genericregs.h"

namespace gcp {
  namespace util {
    
    //------------------------------------------------------------  
    // A class to manage an arraymap pointer
    //------------------------------------------------------------  

    class ArrayMapPtr {
    public:

      ArrayMap* arrayMap_;

      ArrayMapPtr() {
	arrayMap_ = 0;
      }

      ~ArrayMapPtr() {
	if(arrayMap_) {
	  arrayMap_ = del_ArrayMap(arrayMap_);
	}
      }

      void allocateArrayMap() {
	arrayMap_ = new_ArrayMap();
      }

    };

    // An object which any class needing a copy of the GCP Array map
    // should instantiate.  This class manages a single static copy of
    // the GCP array map, which is allocated on the first construction
    // of this class, and deleted only when the last reference to any
    // such object is deleted.

    class ArrayMapBase {
    public:
      
      /**
       * Constructor.
       */
      ArrayMapBase();
      
      /**
       * Copy constructor
       */
      ArrayMapBase(ArrayMapBase& arrayMap);
      ArrayMapBase(const ArrayMapBase& arrayMap);
      void operator=(const ArrayMapBase& arrayMap);
      void operator=(ArrayMapBase& arrayMap);

      // Destructor.

      virtual ~ArrayMapBase();
      
      // Get a reference to the array map

      inline ArrayMap* arrayMap() {
	return arrayMapPtr_.arrayMap_;
      }

    public:

      // The underlying singleton array map

      static ArrayMapPtr arrayMapPtr_;

    }; // End class ArrayMapBase
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_ARRAYMAPBASE_H
