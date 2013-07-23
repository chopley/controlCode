// $Id: Month.h,v 1.1 2010/02/23 17:19:57 eml Exp $

#ifndef GCP_UTIL_MONTH_H
#define GCP_UTIL_MONTH_H

/**
 * @file Month.h
 * 
 * Tagged: Sat Jan 16 11:06:04 NZDT 2010
 * 
 * @version: $Revision: 1.1 $, $Date: 2010/02/23 17:19:57 $
 * 
 * @author tcsh: username: Command not found.
 */
#include <map>

namespace gcp {
  namespace util {

    class Month {
    public:

      /**
       * Constructor.
       */
      Month();

      /**
       * Destructor.
       */
      virtual ~Month();

      static std::string fullMonthName(unsigned iMonth, bool capitalize=true);
      static std::string abbreviatedMonthName(unsigned iMonth, bool capitalize=true);
      static unsigned daysInMonth(unsigned iMonth, unsigned iYear);

    private:

      static std::map<unsigned, std::string> monthNames_;
      static std::map<unsigned, std::string> createNameMap();

      static std::map<unsigned, unsigned> monthDays_;
      static std::map<unsigned, unsigned> createDayMap();

      static void validateMonthIndex(unsigned iMonth);

    }; // End class Month

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_MONTH_H
