#ifndef GCP_UTIL_NETDATAFRAMEHANDLER_H
#define GCP_UTIL_NETDATAFRAMEHANDLER_H

/**
 * @file NetDataFrameHandler.h
 * 
 * Tagged: Sun Apr  4 22:34:52 UTC 2004
 * 
 * @author Erik Leitch
 */
namespace gcp {
  namespace util {
    
    class NetDataFrameHandler : public NetHandler {
    public:
      
      /**
       * Constructor.
       */
      NetDataFrameHandler();
      
      /**
       * Destructor.
       */
      virtual ~NetDataFrameHandler();
      
    private:
    }; // End class NetDataFrameHandler
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_NETDATAFRAMEHANDLER_H
