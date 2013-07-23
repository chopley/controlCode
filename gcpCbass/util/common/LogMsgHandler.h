// $Id: LogMsgHandler.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_LOGMSGHANDLER_H
#define GCP_UTIL_LOGMSGHANDLER_H

/**
 * @file LogMsgHandler.h
 * 
 * Tagged: Mon Feb 12 07:14:18 NZDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author Erik Leitch
 */

#include <iostream>
#include <sstream>
#include <map>

#include "gcp/util/common/Mutex.h"

namespace gcp {
  namespace util {

    class LogMsgHandler {

    public:

      struct LogMsg {

	enum Type {
	  TYPE_UNSPEC,
	  TYPE_ERR,
	  TYPE_MESS
	};

	LogMsg(unsigned seq) {
	  seq_ = seq;
	  lastReadIndex_ = 0;
	  type_ = TYPE_UNSPEC;
	}

	~LogMsg() {}

	std::ostringstream os_;
	unsigned seq_;
	unsigned lastReadIndex_;
	Type type_;
      };

      /**
       * Constructor.
       */
      LogMsgHandler();

      /**
       * Destructor.
       */
      virtual ~LogMsgHandler();

      // Get the next unique sequence number

      unsigned nextSeq();

      // Append a string to an existing message

      void append(unsigned seq, std::string text, LogMsg::Type type=LogMsg::TYPE_UNSPEC);

      void appendWithSpace(unsigned seq, std::string text, LogMsg::Type type=LogMsg::TYPE_UNSPEC);

      // Get a tagged message

      std::string getMessage(unsigned seq);
      std::string readMessage(unsigned seq);

      std::string getNextMessageSubstr(unsigned seq, unsigned maxChars, bool& isLast);


    private:

      Mutex seqLock_;
      unsigned seq_;
      
      std::map<unsigned, LogMsg*> messages_;

      // Return the next message

      void eraseMessage(unsigned seq);

      // Return the next message

      LogMsg* findMessage(unsigned seq);

    }; // End class LogMsgHandler

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_LOGMSGHANDLER_H
