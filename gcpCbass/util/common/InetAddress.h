// $Id: InetAddress.h,v 1.2 2011/08/26 23:59:35 eml Exp $

#ifndef GCP_UTIL_INETADDRESS_H
#define GCP_UTIL_INETADDRESS_H

/**
 * @file InetAddress.h
 * 
 * Tagged: Tue Mar 16 16:30:10 PDT 2010
 * 
 * @version: $Revision: 1.2 $, $Date: 2011/08/26 23:59:35 $
 * 
 * @author tcsh: username: Command not found.
 */
#include <string>

namespace gcp {
  namespace util {

    class InetAddress {
    public:

      /**
       * Constructor.
       */
      InetAddress();

      /**
       * Destructor.
       */
      virtual ~InetAddress();

      static std::string resolveName(std::string name, bool doThrow=true);

    }; // End class InetAddress

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_INETADDRESS_H
