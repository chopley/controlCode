#ifndef GCP_UTIL_MONITORCONDITION_H
#define GCP_UTIL_MONITORCONDITION_H

/**
 * @file MonitorCondition.h
 * 
 * Tagged: Sun Oct 24 17:05:09 PDT 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/DataType.h"
#include "gcp/util/common/DataTypeTruthFn.h"

namespace gcp {
  namespace util {
    
    /**
     * A class for handling a condition for a monitor point.
     */
    class MonitorCondition {
    public:
      
      /**
       * Constructor for no condition.  Matches any condition.
       */
      MonitorCondition(unsigned packetCount       = defaultPacketCount_, 
		       unsigned stablePacketCount = defaultStablePacketCount_,
		       unsigned giveUpPacketCount = defaultGiveUpPacketCount_);
      
      /**
       * Constructor for a single-valued condition.
       *
       * @param op          operator to check with
       *
       * @param delta       If true, check the delta of this value
       *
       * @param packetCount start checking when the packet count after 
       *                    issuing this condition reaches this count.
       *
       * @param stablePacketCount After we start checking, the
       *                          condition must hold true for this
       *                          many consecutive packets before we will 
       *                          report it as true
       *
       * @param giveUpPacketCount If the condition has not become true this 
       *                          many packets _after we start checking_, 
       *                          give up (0 == keep trying forever)
       */
      MonitorCondition(gcp::util::DataTypeTruthFn fn, 
		       gcp::util::DataType op1, 
		       bool delta = false,
		       unsigned packetCount       = defaultPacketCount_,
		       unsigned stablePacketCount = defaultStablePacketCount_,
		       unsigned giveUpPacketCount = defaultGiveUpPacketCount_);
      
      /**
       * Constructor for a dual-valued condition
       */
      MonitorCondition(gcp::util::DataTypeTruthFn fn,
		       gcp::util::DataType op1, 
		       gcp::util::DataType op2,
		       bool delta = false, 
		       unsigned packetCount       = defaultPacketCount_,
		       unsigned stablePacketCount = defaultStablePacketCount_,
		       unsigned giveUpPacketCount = defaultGiveUpPacketCount_);
      
      
      // Equivalent setTo() methods to the constructor versions
      
      void setTo(unsigned packetCount       = defaultPacketCount_, 
		 unsigned stablePacketCount = defaultStablePacketCount_, 
		 unsigned giveUpPacketCount = defaultGiveUpPacketCount_);
      
      void setTo(gcp::util::DataTypeTruthFn fn, 
		 gcp::util::DataType op1, 
		 bool delta = false,
		 unsigned packetCount       = defaultPacketCount_, 
		 unsigned stablePacketCount = defaultStablePacketCount_, 
		 unsigned giveUpPacketCount = defaultGiveUpPacketCount_);
      
      void setTo(gcp::util::DataTypeTruthFn fn,
		 gcp::util::DataType op1, 
		 gcp::util::DataType op2, 
		 bool delta = false,
		 unsigned packetCount       = defaultPacketCount_, 
		 unsigned stablePacketCount = defaultStablePacketCount_, 
		 unsigned giveUpPacketCount = defaultGiveUpPacketCount_);
      
      void operator=(const MonitorCondition& condition);
      
      /**
       * Format this condition
       */
      std::string format(std::string& reg);

      /**
       * Destructor.
       */
      virtual ~MonitorCondition();
      
      // Return true if the condition is satisfied by the passed value
      
      bool isSatisfiedBy(gcp::util::DataType& dataType);
      
      /**
       * Check the packet count to see if it makes sense
       */
      void checkPacketCount(unsigned packetCount, unsigned stablePacketCount, 
			    unsigned giveUpPacketCount);
      
      // Public query functions

      bool isDelta() {return isDeltaCondition_;}
      unsigned nFrame() {return stablePacketCount_;}
      double min() {return op1_.getValAsDouble();}
      double max() {return op2_.getValAsDouble();}

      // Members

      gcp::util::DataTypeTruthFn fn_;
      gcp::util::DataType op1_;
      gcp::util::DataType op2_;

      bool isDeltaCondition_;
      bool first_;
      gcp::util::DataType last_;
      
      unsigned packetCount_;
      unsigned stablePacketCount_;
      unsigned giveUpPacketCount_;
      
      static const unsigned defaultPacketCount_       =  1;
      static const unsigned defaultStablePacketCount_ =  0;
      static const unsigned defaultGiveUpPacketCount_ = 20;
      
    }; // End class MonitorCondition
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_MONITORCONDITION_H
