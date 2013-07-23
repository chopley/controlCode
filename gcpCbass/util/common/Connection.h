// $Id: Connection.h,v 1.1 2010/02/23 17:19:57 eml Exp $

#ifndef GCP_UTIL_CONNECTION_H
#define GCP_UTIL_CONNECTION_H

/**
 * @file Connection.h
 * 
 * Tagged: Fri Dec  4 13:28:34 PST 2009
 * 
 * @version: $Revision: 1.1 $, $Date: 2010/02/23 17:19:57 $
 * 
 * @author tcsh: username: Command not found.
 */
#include <string>

namespace gcp {
  namespace util {

    class Connection {
    public:

      /**
       * Constructor.
       */
      Connection();

      /**
       * Destructor.
       */
      virtual ~Connection();

      bool isReachable(std::string host, unsigned timeOutInSeconds=10);

    private:

    }; // End class Connection

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_CONNECTION_H
