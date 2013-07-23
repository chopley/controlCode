#ifndef EXCEPTION_H
#define EXCEPTION_H

/**
 * @file Exception.h
 * 
 * Tagged: Fri Nov 14 12:39:33 UTC 2003
 * 
 * @author Erik Leitch
 */
// System includes

#include <iostream>
#include <sstream>
#include <string>

#include "gcp/util/common/ErrHandler.h"
#include "gcp/util/common/FunctionName.h"
#include "gcp/util/common/Directives.h"
#include "gcp/util/common/IoLock.h"
#include "gcp/util/common/Logger.h"

// Create an Exception class in namespace carma::antenna::gcp

namespace gcp {
  namespace util {

    class LogStream;

    class Exception {
      
    public:
      
      /**
       * Construct an Exception with a detailed message.
       *
       * @param str String describing error.
       * @param filename where exception originated.
       * @param lineNumber exception occurred on.
       */
      Exception(std::string str, const char * filename,  
		const int lineNumber, bool report);
      
      /**
       * Construct an Error with a detailed message.
       * 
       * @param os ostringstream containing message
       * @param filename where exception originated.
       * @param lineNumber exception occurred on.
       */
      Exception(std::ostringstream& os, const char * filename, 
		const int lineNumber, bool report);
      
      /** 
       * Constructor with an LogStream&.
       */
      Exception(gcp::util::LogStream& ls, 
		const char* filename, const int lineNumber, bool report);
      
      /** 
       * Constructor with an LogStream*.
       */
      Exception(gcp::util::LogStream* ls, 
		const char* filename, const int lineNumber, bool report);
      

      /**
       * Destructor
       */
      virtual ~Exception();

      /**
       * Report error to standard err.
       * Reports error to standard error by printing the
       * error message, filename and line number.
       */
      inline void report() {}      

      /**
       * Report error to standard err.
       * Reports error to standard error by printing the
       * error message, filename and line number.
       */
      inline void report(std::string& what) 
	{
	  gcp::util::IoLock::lockCerr();
	  std::cerr << what;
	  gcp::util::IoLock::unlockCerr();
	}      

      /**
       * Report error to standard err.
       * Reports error to standard error by printing the
       * error message, filename and line number.
       */
      inline void report(std::string what) 
	{
	  gcp::util::IoLock::lockCerr();
	  std::cerr << what;
	  gcp::util::IoLock::unlockCerr();
	}      

      inline const char* what() { 
	return message_.c_str();
      }
      
    private: 
      
      std::string message_;

    }; // End class Exception
    
  } // namespace util
} // namespace gcp

#define Error(x) gcp::util::Exception((x), __FILE__, __LINE__, true)
#define ErrorNoReport(x) gcp::util::Exception((x), __FILE__, __LINE__, false)
#define ErrorDef(x,y) gcp::util::Exception (x)((y), __FILE__, __LINE__, true)

#ifdef ThrowError
#undef ThrowError
#endif

#define ThrowError(text) \
{\
  std::ostringstream _macroOs; \
  _macroOs << text;\
  gcp::util::ErrHandler::throwError(_macroOs, __FILE__, __LINE__, \
     gcp::util::FunctionName::noArgs(__PRETTY_FUNCTION__).c_str(), \
     true, false, false);\
}

#ifdef ThrowSimpleError
#undef ThrowSimpleError
#endif

#define ThrowSimpleError(text) \
{\
  std::ostringstream _macroOs; \
  _macroOs << text;\
  gcp::util::ErrHandler::throwError(_macroOs, __FILE__, __LINE__, \
     gcp::util::FunctionName::noArgs(__PRETTY_FUNCTION__).c_str(), \
     true, true, false);\
}

#ifdef ThrowSysError
#undef ThrowSysError
#endif

#define ThrowSysError(text) \
{\
  std::ostringstream _macroOs; \
  _macroOs << text;\
  gcp::util::ErrHandler::throwError(_macroOs, __FILE__, __LINE__, \
     gcp::util::FunctionName::noArgs(__PRETTY_FUNCTION__).c_str(), \
     true, false, true);\
}

#ifdef ReportError
#undef ReportError
#endif

#define ReportError(text) \
{\
  std::ostringstream _macroOs; \
  _macroOs << text;\
  gcp::util::ErrHandler::report(_macroOs, __FILE__, __LINE__, \
     gcp::util::FunctionName::noArgs(__PRETTY_FUNCTION__).c_str(), \
     true, false, false);\
}

#ifdef ReportSimpleError
#undef ReportSimpleError
#endif

#define ReportSimpleError(text) \
{\
  std::ostringstream _macroOs; \
  _macroOs << text;\
  gcp::util::ErrHandler::report(_macroOs, __FILE__, __LINE__, \
     gcp::util::FunctionName::noArgs(__PRETTY_FUNCTION__).c_str(), \
     true, true, false);\
}

#ifdef ReportSysError
#undef ReportSysError
#endif

#define ReportSysError(text) \
{\
  std::ostringstream _macroOs; \
  _macroOs << text;\
  gcp::util::ErrHandler::report(_macroOs, __FILE__, __LINE__, \
     gcp::util::FunctionName::noArgs(__PRETTY_FUNCTION__).c_str(), \
     true, false, true);\
}

#ifdef ReportSimpleSysError
#undef ReportSimpleSysError
#endif

#define ReportSimpleSysError(text) \
{\
  std::ostringstream _macroOs; \
  _macroOs << text;\
  gcp::util::ErrHandler::report(_macroOs, __FILE__, __LINE__, \
     gcp::util::FunctionName::noArgs(__PRETTY_FUNCTION__).c_str(), \
     true, true, true);\
}

#ifdef ReportMessage
#undef ReportMessage
#endif

#define ReportMessage(text) \
{\
  std::ostringstream _macroOs; \
  _macroOs << text;\
  gcp::util::ErrHandler::report(_macroOs, __FILE__, __LINE__, \
     gcp::util::FunctionName::noArgs(__PRETTY_FUNCTION__).c_str(), \
     false, true, false);\
}

#ifdef LogMessage
#undef LogMessage
#endif

#define LogMessage(error, text) \
{\
  std::ostringstream _macroOs; \
  _macroOs << text;\
  gcp::util::ErrHandler::log(_macroOs, __FILE__, __LINE__, \
     gcp::util::FunctionName::noArgs(__PRETTY_FUNCTION__).c_str(), \
     false, true, false);\
}

#define COUT(statement) \
{\
    std::ostringstream _macroOs; \
    _macroOs << statement << std::endl; \
    gcp::util::Logger::printToStdout(_macroOs.str()); \
}

#define CTOUT(statement) \
{\
     gcp::util::TimeVal _macroTimeVal;\
    _macroTimeVal.setToCurrentTime();\
    std::ostringstream _macroOs; \
    _macroOs << _macroTimeVal << ": " << statement << std::endl; \
    gcp::util::Logger::printToStdout(_macroOs.str()); \
}

#define CERR(statement) \
{\
    std::ostringstream _macroOs; \
    _macroOs << statement << std::endl; \
    gcp::util::Logger::printToStderr(_macroOs.str()); \
}

#define CTERR(statement) \
{\
    gcp::util::TimeVal _macroTimeVal;\
    _macroTimeVal.setToCurrentTime();\
    std::ostringstream _macroOs; \
    _macroOs << _macroTimeVal << ": " << statement << std::endl; \
    gcp::util::Logger::printToStderr(_macroOs.str()); \
}

#endif
