// $Id: NewRtcNetMsg.h,v 1.1.1.1 2009/07/06 23:57:26 eml Exp $

#ifndef GCP_UTIL_NEWRTCNETMSG_H
#define GCP_UTIL_NEWRTCNETMSG_H

/**
 * @file NewRtcNetMsg.h
 * 
 * Tagged: Thu Jul  7 17:44:19 PDT 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:26 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/NetStruct.h"

namespace gcp {
  namespace util {

    class NewNetMsg;

    // A network command received from the real-time controller
    //
    // Note: to use this class, specific experiments must define an
    // allocator newSpecificNetMsg(), which returns a pointer to an
    // inheritor from NewNetMsg

    NewNetMsg* newSpecificNetMsg();

    class NewRtcNetMsg : public gcp::util::NetStruct {
    public:

      unsigned int antenna; // An antenna-identifier
      NewNetMsg* msg;          // An experiment-specific message

      /**
       * Constructor.
       */
      NewRtcNetMsg();

      /**
       * Copy constructor
       */
      NewRtcNetMsg(const NewRtcNetMsg&);
      NewRtcNetMsg(NewRtcNetMsg&);
      NewRtcNetMsg& operator=(const NewRtcNetMsg& netStruct);
      NewRtcNetMsg& operator=(NewRtcNetMsg& netStruct);

      /**
       * Destructor.
       */
      virtual ~NewRtcNetMsg();

    }; // End class NewRtcNetMsg

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_NEWRTCNETMSG_H
