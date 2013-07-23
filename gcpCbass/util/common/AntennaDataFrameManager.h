#ifndef GCP_UTIL_ANTENNADATAFRAMEMANAGER_H
#define GCP_UTIL_ANTENNADATAFRAMEMANAGER_H

/**
 * @file AntennaDataFrameManager.h
 * 
 * Tagged: Sat Mar 20 05:20:30 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/RegMapDataFrameManager.h"
#include "gcp/util/common/AntennaDataFrame.h"

// C header files from the array control code

#include "gcp/control/code/unix/libunix_src/common/genericregs.h"

namespace gcp {
  namespace util {
    
    class AntennaDataFrameManager : public gcp::util::RegMapDataFrameManager {
    public:
      
      /**
       * Constructors
       */
      AntennaDataFrameManager(bool archivedOnly=false);
      
      AntennaDataFrameManager(const gcp::util::AntNum& antNum,
			      bool archivedOnly=false);
      
      AntennaDataFrameManager(gcp::util::AntNum* antNum,
			      bool archivedOnly=false);
      
      AntennaDataFrameManager(AntennaDataFrameManager& fm);
      
      /**
       * Destructor.
       */
      virtual ~AntennaDataFrameManager();
      
      /**
       * Set the antenna number associated with this dataframe.
       * @throws Exception
       */
      void setAnt(gcp::util::AntNum::Id antennaId);
      
      /**
       * Return the antenna number associated with this dataframe.
       *
       * @throws Exception
       */
      unsigned int getAntIntId();
      
      /**
       * Return the antenna descriptor associated with this dataframe.
       */
      AntNum getAnt();
      
      /**
       * Initialize this object.
       */
      void initialize(bool archivedOnly = false);
      
      /**
       * Assignment operators
       */
      void operator=(RegMapDataFrameManager& fm);
      void operator=(AntennaDataFrameManager& fm);
      
    private:
      
      AntennaDataFrame* antFrame_; // Pointer to the frame managed by
      // this object
      
    }; // End class AntennaDataFrameManager
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_ANTENNADATAFRAMEMANAGER_H
