#ifndef GCP_UTIL_LABJACKMSG_H
#define GCP_UTIL_LABJACKMSG_H

/**
 * @file DlpUsbtTermal.h
 *
 * Tagged: Tue Oct 16 13:01:01 PDT 2007
 *
 * @version: $Revision: 1.1 $, $Date: 2013/01/17 16:16:35 $
 *
 * @author username: Command not found.
 */
#include "gcp/util/common/CondVar.h"
#include "gcp/util/common/SerialClient.h"
#include "gcp/util/common/SpawnableTask.h"
#include "gcp/util/common/GenericTaskMsg.h"
#include "gcp/util/common/TimeOut.h"
#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"


#include <sstream>
#define DATA_MAX_LEN 200

namespace gcp {
  namespace util {

    class LabjackMsg {
    public:


      /**
       * Enumerate supported commands
       */
      enum Request {
	INVALID,
	SET_DATA_TYPE,
	SET_UNITS,
	QUERY_TEMP,
	QUERY_VOLTAGE,
	QUERY_ALL_TEMP,
	QUERY_ALL_VOLT,
      };

      /** 
       * Index pertaining to request types
       */
      unsigned char request_;
      
      /**
       *  Command to be issued
       */ 
      std::string messageToSend_;
      
      /**
       * Response Received
       */
      char responseReceived_[DATA_MAX_LEN];

      /** 
       * Whether a response is expected
       */ 
      bool expectsResponse_;

      /**
       * Response Value
       */
      float responseValue_;
      float responseValueVec_[NUM_TEMP_SENSORS];

      /**
       * The size of the command to send.
       */
      unsigned short cmdSize_; 
      

    }; //End DlpUsbThermalMsg class.

  }; //End namespace util
}; // End namesapce gcp

#endif // End #ifndef

