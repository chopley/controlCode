// $Id: DoubleBufferVec.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_DOUBLEBUFFERVEC_H
#define GCP_UTIL_DOUBLEBUFFERVEC_H

/**
 * @file DoubleBufferVec.h
 * 
 * Tagged: Tue Jan 30 03:11:36 NZDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/DoubleBuffer.h"

#include <iostream>
#include <valarray>

namespace gcp {
  namespace util {

    template<class type>
      class DoubleBufferVec : public DoubleBuffer {
      public:

      /**
       * Constructor.
       */
      DoubleBufferVec(unsigned n=0) 
	{
	  resize(n);
	}

      type* getReadBuffer()
	{
	  return (type*)grabReadBuffer();
	}

      type* getWriteBuffer()
	{
	  return (type*)grabWriteBuffer();
	}

      /**
       * Destructor.
       */
      virtual ~DoubleBufferVec() {};

      unsigned size() {
	return data1_.size();
      }

      void resize(unsigned n) {

	data1_.resize(n);
	data2_.resize(n);

	buf1_.data_ = &data1_[0];
	buf2_.data_ = &data2_[0];
      }

      private:

      std::valarray<type> data1_;
      std::valarray<type> data2_;

    }; // End class DoubleBufferVec

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_DOUBLEBUFFERVEC_H
