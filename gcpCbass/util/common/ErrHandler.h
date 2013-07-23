// $Id: ErrHandler.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_ERRHANDLER_H
#define GCP_UTIL_ERRHANDLER_H

/**
 * @file ErrHandler.h
 * 
 * Tagged: Mon Apr 10 13:32:05 PDT 2006
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author Erik Leitch
 */
#include <sstream>

#define ERR_HANDLER_FN(fn) \
  void (fn)(std::ostringstream& os, const char* fileName, const int lineNumber, const char* functionName, bool isErr, bool isSimple, bool isSysErr)

namespace gcp {
  namespace util {

    class ErrHandler {
    public:

      /**
       * Constructor.
       */
      ErrHandler();

      /**
       * Destructor.
       */
      virtual ~ErrHandler();

      static ERR_HANDLER_FN(defaultThrowFn);
      static ERR_HANDLER_FN(defaultReportFn);
      static ERR_HANDLER_FN(defaultLogFn);

      // Install user-defined error handling functions

      static void installThrowFn(ERR_HANDLER_FN(*throwFn));
      static void installReportFn(ERR_HANDLER_FN(*reportFn));
      static void installLogFn(ERR_HANDLER_FN(*logFn));

      static ERR_HANDLER_FN(throwError);
      static ERR_HANDLER_FN(report);
      static ERR_HANDLER_FN(log);

    private:

      static ERR_HANDLER_FN(*throwFn_);
      static ERR_HANDLER_FN(*reportFn_);
      static ERR_HANDLER_FN(*logFn_);

    }; // End class ErrHandler

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_ERRHANDLER_H
