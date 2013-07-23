// $Id: InitScript.h,v 1.1.1.1 2009/07/06 23:57:07 eml Exp $

#ifndef GCP_CONTROL_INITSCRIPT_H
#define GCP_CONTROL_INITSCRIPT_H

/**
 * @file InitScript.h
 * 
 * Tagged: Fri Jul  8 12:11:48 PDT 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:07 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Directives.h"
#include <string>

namespace gcp {
  namespace control {

    class InitScript {
    public:

      static std::string initScript();

      /**
       * Constructor.
       */
      InitScript();

      /**
       * Destructor.
       */
      virtual ~InitScript();

    private:
    }; // End class InitScript

  } // End namespace control
} // End namespace gcp



#endif // End #ifndef GCP_CONTROL_INITSCRIPT_H
