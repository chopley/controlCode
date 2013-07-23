// $Id: SpecificNetCmd.h,v 1.1.1.1 2009/07/06 23:57:07 eml Exp $

#ifndef GCP_CONTROL_SPECIFICNETCMD_H
#define GCP_CONTROL_SPECIFICNETCMD_H

/**
 * @file SpecificNetCmd.h
 * 
 * Tagged: Mon Jul 11 12:35:13 PDT 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:07 $
 * 
 * @author Erik Leitch
 */
#include "NewNetCmd.h"

namespace gcp {
  namespace control {
 
    class SpecificNetCmd : public gcp::control::NewNetCmd {
    public:
      /*----------------------------------------------------------------
       * Define the ids of controller to control-program commands along with
       * the corresponding message containers
       */
      typedef enum {
      } NetCmdId;
    
      /**
       * Constructor.
       */
      SpecificNetCmd();
    
      /**
       * Destructor.
       */
      virtual ~SpecificNetCmd();
    
      // Commands

    };
  
  } // End namespace control
} // End namespace gcp



#endif // End #ifndef GCP_CONTROL_SPECIFICNETCMD_H
