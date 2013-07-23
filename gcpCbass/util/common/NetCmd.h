#ifndef GCP_UTIL_NETCMD_H
#define GCP_UTIL_NETCMD_H

/**
 * @file NetCmd.h
 * 
 * Tagged: Wed Mar 17 19:42:02 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"

#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/Percent.h"
#include "gcp/util/common/Pressure.h"
#include "gcp/util/common/Temperature.h"

namespace gcp {
  namespace util {
    
    class NetCmd {
    public:
      
      /**
       * Constructor.
       */
      NetCmd(gcp::control::RtcNetCmd rtc, gcp::control::NetCmdId opcode);
      NetCmd();
      
      /**
       * Destructor.
       */
      virtual ~NetCmd();
      
      gcp::control::RtcNetCmd rtc_;
      
      gcp::control::NetCmdId opcode_;
      
      // True if this is an initialization command
      
      bool init_;
      
      void packAtmosCmd(Temperature& airTemp, Percent& humidity, Pressure& pressure,
			AntNum::Id antennas = AntNum::ANTALL);
      
    }; // End class NetCmd
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_NETCMD_H
