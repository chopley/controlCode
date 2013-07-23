#ifndef GCP_UTIL_NETANTENNADATAFRAMEHANDLER_H
#define GCP_UTIL_NETANTENNADATAFRAMEHANDLER_H

/**
 * @file NetAntennaDataFrameHandler.h
 * 
 * Tagged: Mon Apr  5 00:19:48 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/AntennaDataFrameManager.h"
#include "gcp/util/common/NetHandler.h"

namespace gcp {
  namespace util {
    
    class NetAntennaDataFrameHandler : public NetHandler {
    public:
      
      /**
       * Constructor.
       */
      NetAntennaDataFrameHandler();
      
      /**
       * Destructor.
       */
      virtual ~NetAntennaDataFrameHandler();
      
      /**
       * Return a pointer to our data frame manager.
       */
      AntennaDataFrameManager* getFrame();
      
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
      unsigned int getAnt();
      
    private:
      
      AntennaDataFrameManager frame_;
      
    }; // End class NetAntennaDataFrameHandler
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_NETANTENNADATAFRAMEHANDLER_H
