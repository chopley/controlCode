// $Id: ModemPager.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_MODEMPAGER_H
#define GCP_UTIL_MODEMPAGER_H

/**
 * @file ModemPager.h
 * 
 * Tagged: Fri Feb 16 12:31:26 NZDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author Erik Leitch
 */

#include <iostream>


#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/GenericTaskMsg.h"
#include "gcp/util/common/Runnable.h"
#include "gcp/util/common/SpawnableTask.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/control/code/unix/libunix_src/common/lprintf.h"

namespace gcp {
  namespace util {

    // A utility class for sending messages to the ModemPager task

    class ModemPagerMsg : public GenericTaskMsg {
    public:

      enum MsgType {
	ACK,
	ALIVE,
	ERROR,
	ENABLE,
	RESET,
	START,
	STATUS,
	STOP
      };

      union {
	bool enable;
      } body;

      // A type for this message

      MsgType type;
    };

    /**
     * Class for communicating with Steffen's analog modem pager
     */
    class ModemPager : public SpawnableTask<ModemPagerMsg> {
    public:

      enum State {
	STATE_NORM,
	STATE_ESC
      };

      enum SpecialChars {
	ESC  =  27,
	TERM = 109
      };

      /**
       * Constructor.
       */
      ModemPager();

      /**
       * Constructor.
       */
      ModemPager(FILE* stdoutStream, LOG_DISPATCHER(*stdoutDispatcher), 
		 void* stdoutContext,
		 FILE* stderrStream, LOG_DISPATCHER(*stderrDispatcher), 
		 void* stderrContext);

      /**
       * Destructor.
       */
      virtual ~ModemPager();

      // Activate the pager

      void start();
      void stop();
      void acknowledge();
      void activate(std::string message);
      void enable(bool enable);
      void sendAlive();
      void requestStatus();
      void reset();

    private:

      FILE* stdoutStream_;
      FILE* stderrStream_;
      LOG_DISPATCHER(*stdoutDispatcher_);
      LOG_DISPATCHER(*stderrDispatcher_);
      void* stdoutContext_;
      void* stderrContext_;

      std::string errorMessage_;
      Mutex msgLock_;

      TimeVal aliveTimeOut_;

      void serviceMsgQ(void);
      void processMsg(ModemPagerMsg* msg);

      void executeAcknowledge();
      void executeAlive();
      void executeError();
      void executeEnable(bool enable);
      void executeReset();
      void executeStart();
      void executeStatus();
      void executeStop();

      void spawnAndCapture(std::string script, bool log=false);

    }; // End class ModemPager

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_MODEMPAGER_H
