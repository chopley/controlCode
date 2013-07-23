#ifndef GCP_TRANSLATOR_ARRAYDATAFRAMEMANAGER_H
#define GCP_TRANSLATOR_ARRAYDATAFRAMEMANAGER_H

/**
 * @file ArrayDataFrameManager.h
 * 
 * Tagged: Sat Mar 20 05:20:30 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/ArrayMapDataFrameManager.h"
#include "gcp/util/common/AntennaDataFrameManager.h"
#include "gcp/util/common/Coord.h"

// C header files from the array control code

#include "gcp/control/code/unix/libunix_src/common/genericregs.h"

namespace gcp {
  namespace util {
    
    class ArrayDataFrameManager : public ArrayMapDataFrameManager {
    public:
      
      /**
       * Constructors
       */
      ArrayDataFrameManager(bool archivedOnly=false, ArrayMap* arrayMap=NULL);
      ArrayDataFrameManager(ArrayDataFrameManager& fm);
      
      /**
       * Destructor.
       */
      virtual ~ArrayDataFrameManager();
      
      /**
       * Initialize this object.
       */
      void initialize(ArrayMap* arrayMap, bool archivedOnly_=false);
      
      /**
       * Assignment operators
       */
      void operator=(ArrayDataFrameManager& fm);
      
      /**
       * Write an antenna data frame into our array frame
       */
      void writeAntennaRegMap(AntennaDataFrameManager& fm, bool lockFrame);
      
      /**
       * Write a data frame into our array frame
       */      
      void writeGenericRegMap(RegMapDataFrameManager& fm, bool lockFrame, std::string regMapName);    
      
      /**
       * Find the register map corresponding to an antenna data frame
       */
      ArrRegMap* findAntennaRegMap(AntennaDataFrameManager& fm);
      
    }; // End class ArrayDataFrameManager
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_TRANSLATOR_ARRAYDATAFRAMEMANAGER_H
