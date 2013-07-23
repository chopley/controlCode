// $Id: SpecificNetMsg.h,v 1.1.1.1 2009/07/06 23:57:24 eml Exp $

#ifndef GCP_UTIL_SPECIFICNETMSG_H
#define GCP_UTIL_SPECIFICNETMSG_H

/**
 * @file SpecificNetMsg.h
 * 
 * Tagged: Tue Jun 28 16:15:55 PDT 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:24 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/NewNetMsg.h"
#include "gcp/util/common/NetStruct.h"

namespace gcp {
  namespace util {
  
    class SpecificNetMsg : public NewNetMsg {
    public:
    
      enum {
      };

      /**
       * Constructor.
       */
      SpecificNetMsg();
      
      /**
       * Destructor.
       */
      virtual ~SpecificNetMsg();
      
    }; // End class SpecificNetMsg
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_SPECIFICNETMSG_H
