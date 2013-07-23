// $Id: TransactionStatus.h,v 1.1.1.1 2009/07/06 23:57:07 eml Exp $

#ifndef GCP_CONTROL_TRANSACTIONSTATUS_H
#define GCP_CONTROL_TRANSACTIONSTATUS_H

/**
 * @file TransactionStatus.h
 * 
 * Tagged: Thu Jun 23 10:33:59 PDT 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:07 $
 * 
 * @author Erik Leitch
 */
namespace gcp {
  namespace control {

    class TransactionStatus {
    public:

      enum {
	TRANS_FRAME,
	TRANS_GRAB,
	TRANS_PMAC,
        TRANS_BENCH,
	TRANS_MARK,
	TRANS_SCAN,
	TRANS_SETREG,
	TRANS_TVOFF,
	TRANS_SCRIPT,
	TRANS_LAST // This should always come last
      };

      struct Transaction {

	unsigned seq_;    // The sequence number of the last positioning
	//  command sent to the caltert module
	unsigned done_;   // A bitmask of completion status for all
	// antennas.  Each bit will be set to 1 when the
	// corresponding antenna has completed

	Transaction();
      };

      /**
       * Constructor.
       */
      TransactionStatus();

      /**
       * Destructor.
       */
      virtual ~TransactionStatus();

      // Return the completion status of the requested transaction

      virtual unsigned int& done(unsigned transId);

      virtual const unsigned int seq(unsigned transId);

      // Return the next sequence number

      virtual unsigned nextSeq(unsigned transId);

    protected:

      Transaction frame_;
      Transaction grab_;
      Transaction pmac_;
      Transaction bench_;
      Transaction mark_;
      Transaction scan_;
      Transaction setreg_;
      Transaction tvoff_;
      Transaction script_;

    }; // End class TransactionStatus

  } // End namespace control
} // End namespace gcp



#endif // End #ifndef GCP_CONTROL_TRANSACTIONSTATUS_H
