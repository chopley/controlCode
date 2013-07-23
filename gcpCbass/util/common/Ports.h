// $Id: Ports.h,v 1.2 2009/08/17 21:18:31 eml Exp $

#ifndef GCP_UTIL_PORTS_H
#define GCP_UTIL_PORTS_H

/**
 * @file Ports.h
 * 
 * Tagged: Sun Oct  9 13:45:59 PDT 2005
 * 
 * @version: $Revision: 1.2 $, $Date: 2009/08/17 21:18:31 $
 * 
 * @author Erik Leitch
 */
#include <string>

#include "gcp/util/common/Directives.h"

#if DIR_IS_STABLE
#define PORT_OFFSET 5000+gcp::util::Ports::expOffset()
#else
#define PORT_OFFSET 6000+gcp::util::Ports::expOffset()
#endif

/*
 * The TCP/IP port on which the control program listens for connection
 * requests from the translator control process.
 */
#define CP_RTC_PORT_BASE 440
#define CP_RTC_PORT CP_RTC_PORT_BASE+PORT_OFFSET

/*
 * The TCP/IP port on which the control-program listens for external
 * control clients.
 */
#define CP_CONTROL_PORT_BASE 441
#define CP_CONTROL_PORT CP_CONTROL_PORT_BASE+PORT_OFFSET

/*
 * The TCP/IP port on which the control program listens for external
 * monitor clients.
 */
#define CP_MONITOR_PORT_BASE 442
#define CP_MONITOR_PORT CP_MONITOR_PORT_BASE+PORT_OFFSET

/*
 * The TCP/IP port on which the control program listens for external
 * image monitor clients.
 */
#define CP_IM_MONITOR_PORT_BASE 443
#define CP_IM_MONITOR_PORT CP_IM_MONITOR_PORT_BASE+PORT_OFFSET

/*
 * The TCP/IP port on which that the control program listens for
 * connection requests from the translator scanner process.
 */
#define CP_RTS_PORT_BASE 444
#define CP_RTS_PORT CP_RTS_PORT_BASE+PORT_OFFSET

/*
 * The TCP/IP port on which the control program listens for
 * connection requests from the translator optcam process.
 */
#define CP_RTO_PORT_BASE 445
#define CP_RTO_PORT CP_RTO_PORT_BASE+PORT_OFFSET

/*
 * The TCP/IP port on which the translator listens for control
 * connections from antenna computers.
 */
#define TRANS_ANT_CONTROL_PORT_BASE 446
#define TRANS_ANT_CONTROL_PORT TRANS_ANT_CONTROL_PORT_BASE+PORT_OFFSET

/*
 * The TCP/IP port on which the translator listens for scanner
 * connections from antenna computers.
 */
#define TRANS_ANT_SCANNER_PORT_BASE 447
#define TRANS_ANT_SCANNER_PORT TRANS_ANT_SCANNER_PORT_BASE+PORT_OFFSET

/*
 * The TCP/IP port on which the translator listens for control
 * connections from the frame grabber
 */
#define TRANS_GRABBER_CONTROL_PORT_BASE 448
#define TRANS_GRABBER_CONTROL_PORT TRANS_GRABBER_CONTROL_PORT_BASE+PORT_OFFSET

/*
 * The TCP/IP port on which the weather server listens for connection
 * requests
 */
#define TRANS_WEATHER_PORT_BASE 449
#define TRANS_WEATHER_PORT TRANS_WEATHER_PORT_BASE+PORT_OFFSET

/*
 * The TCP/IP port on which the submm tipper server listens for
 * connection requests
 */
#define TIPPER_SERVER_PORT_BASE 450
#define TIPPER_SERVER_PORT TIPPER_SERVER_PORT_BASE+PORT_OFFSET

/*
 * The TCP/IP port on which the weather station server listens for
 * connection requests
 */
#define WX_SERVER_PORT_BASE 451
#define WX_SERVER_PORT WX_SERVER_PORT_BASE+PORT_OFFSET

namespace gcp {
  namespace util {
    class Ports {
    public:

      static unsigned base(bool stable=DIR_IS_STABLE);
      static unsigned expOffset();
      static unsigned expOffset(std::string exp);

      // Port on which the Tipper server listens for client
      // connections

      static unsigned tipperPort(std::string exp);

      // Port on which the weather station server listens for client
      // connections

      static unsigned wxPort(std::string exp);

      // Ports on which the Control program listens for client
      // connections

      static unsigned controlControlPort(std::string exp);
      static unsigned controlMonitorPort(std::string exp);
      static unsigned controlImMonitorPort(std::string exp);

      static unsigned controlControlPort(bool stable=DIR_IS_STABLE);
      static unsigned controlMonitorPort(bool stable=DIR_IS_STABLE);
      static unsigned controlImMonitorPort(bool stable=DIR_IS_STABLE);

#if 0
      static unsigned controlOptcamPort(std::string exp);
      static unsigned controlMediatorPort(std::string exp);

      // Ports on which the mediator program listens for client
      // connections

      static unsigned mediatorAntennaControlPort(std::string exp);
      static unsigned mediatorAntennaMonitorPort(std::string exp);
      static unsigned mediatorGrabberControlPort(std::string exp);
#endif
    }; // End class Ports

  } // End namespace util
} // End namespace gcp

#endif // End #ifndef GCP_UTIL_PORTS_H



