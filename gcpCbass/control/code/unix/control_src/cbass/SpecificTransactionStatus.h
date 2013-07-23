// $Id: SpecificTransactionStatus.h,v 1.1.1.1 2009/07/06 23:57:07 eml Exp $

#ifndef GCP_CONTROL_SPECIFICTRANSACTIONSTATUS_H
#define GCP_CONTROL_SPECIFICTRANSACTIONSTATUS_H

/**
 * @file SpecificTransactionStatus.h
 * 
 * Tagged: Thu Jun 23 10:50:53 PDT 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:07 $
 * 
 * @author Erik Leitch
 */
#include "TransactionStatus.h"

namespace gcp {
  namespace control {

    class SpecificTransactionStatus : public TransactionStatus {
    public:

      enum {
      };

      /**
       * Constructor.
       */
      SpecificTransactionStatus();

      /**
       * Destructor.
       */
      virtual ~SpecificTransactionStatus();

      unsigned int& done(unsigned int transId);

      const unsigned int seq(unsigned int transId);

      unsigned int nextSeq(unsigned int transId);

    private:

    }; // End class SpecificTransactionStatus

  } // End namespace control
} // End namespace gcp



#endif // End #ifndef GCP_CONTROL_SPECIFICTRANSACTIONSTATUS_H
