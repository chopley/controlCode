// $Id: Pathname.h,v 1.1 2010/02/23 17:19:58 eml Exp $

#ifndef GCP_UTIL_PATHNAME_H
#define GCP_UTIL_PATHNAME_H

/**
 * @file Pathname.h
 * 
 * Tagged: Wed Feb  3 11:15:31 NZDT 2010
 * 
 * @version: $Revision: 1.1 $, $Date: 2010/02/23 17:19:58 $
 * 
 * @author username: Command not found.
 */

#include <iostream>

namespace gcp {
  namespace util {

    class Pathname {
    public:

      /**
       * Constructor.
       */
      Pathname();

      /**
       * Destructor.
       */
      virtual ~Pathname();

      // Return an expanded version of the directory and filename,
      // explicitly filling in for relative paths, symbolic home
      // directories, and environment variables

      static std::string expand(std::string dir, std::string file);

    private:

      // Read the user name at the start of a directory name and
      // replace it by the home directory of the user. Record the
      // result in buffer[].
      
      static void expandHomeDir(char *dir, size_t buflen, char *buffer);
      
      // Read the environment variable at the start of a directory name
      // and replace it by its value. Record the result in buffer[].
      
      static void expandEnvVar(char *dir, size_t buflen, char *buffer);
      
    }; // End class Pathname

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_PATHNAME_H
