#ifndef GCP_UTIL_REGDATE_H
#define GCP_UTIL_REGDATE_H

/**
 * @file RegDate.h
 * 
 * Tagged: Tue Oct 12 09:13:47 PDT 2004
 * 
 * @author Erik Leitch
 */
#include <iostream>
#include <sstream>

#include "gcp/util/common/TimeVal.h"

namespace gcp {
  namespace util {
    
    class RegDate {
    public:
      
      class Data {
      public:
	unsigned dayNo_;
	unsigned mSec_;
	
	friend std::ostream& operator<<(std::ostream& os, Data& data);
	void operator+=(unsigned mSec);
	void operator=(TimeVal& tVal);
      };

      
      /**
       * Constructor.
       */
      RegDate(unsigned dayNo, unsigned mSec);
      RegDate(Data& data);
      RegDate(TimeVal& timeVal);
      RegDate();
      
      void setMjd(double mjd, bool doRound=false);
      void setDate(unsigned dayNo, unsigned mSec);
      void setDayNumber(unsigned dayNo);
      void setMilliSeconds(unsigned mSec);
      void setToCurrentTime(); 
      
      /**
       * Destructor.
       */
      virtual ~RegDate();
      
      /**
       * Output operators
       */
      friend std::ostream& operator<<(std::ostream& os, RegDate& date);
      std::string str();
      
      bool operator==(RegDate& date);
      bool operator>(RegDate& date);
      bool operator>=(RegDate& date);
      bool operator<(RegDate& date);
      bool operator<=(RegDate& date);
      
      RegDate operator-(const RegDate& date);
      RegDate operator+(const RegDate& date);
      RegDate operator/(unsigned int divisor);
      void operator+=(const RegDate& date);
      void operator-=(const RegDate& date);

      void operator=(RegDate::Data& data);
      void operator=(const RegDate& date);
      void operator=(RegDate& date);
      void operator=(TimeVal& tVal);
      
      inline Data* data() {
	return &data_;
      }
      
      double mjd();
      double timeInHours();
      double timeInSeconds();
      
      inline unsigned dayNo() {
	return data_.dayNo_;
      }
      
      inline unsigned mSec() {
	return data_.mSec_;
      }
      
#if 0
      inline TimeVal timeVal() {
        TimeVal time;
        time.setMjd(data_.dayNo_, data_.mSec_);
        return time;
      }
#endif      

      inline TimeVal& timeVal() {
	return timeVal_;
      }

      // Update internal time members from our timeval member

      void updateFromTimeVal();

      void initialize();
      
      static const unsigned milliSecondsPerDay_ = 24*3600*1000;
      
    private:
      
      Data data_;
      TimeVal timeVal_;
      
    }; // End class RegDate
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_REGDATE_H
