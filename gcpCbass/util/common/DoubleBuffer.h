// $Id: DoubleBuffer.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_DOUBLEBUFFER_H
#define GCP_UTIL_DOUBLEBUFFER_H

/**
 * @file DoubleBuffer.h
 * 
 * Tagged: Sun Jan 14 16:44:37 NZDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author Erik Leitch
 */

#include <iostream>

#include "gcp/util/common/Mutex.h"

namespace gcp {

  namespace util {

    class DoubleBuffer {
    public:

      //-----------------------------------------------------------------------
      // A struct for associated a buffer with a lock
      //-----------------------------------------------------------------------

      struct BufferLock {

	// public members

	void* data_;
	gcp::util::Mutex lock_;

	// Constructor

	BufferLock() {
	  data_ = 0;
	}

	void lock() {
	  lock_.lock();
	}

	void unlock() {
	  lock_.unlock();
	}

      };

      //-----------------------------------------------------------------------
      // Methods of DoubleBuffer class
      //-----------------------------------------------------------------------

      /**
       * Constructor.
       */
      DoubleBuffer();

      /**
       * Const Assignment Operator.
       */
      void operator=(const DoubleBuffer& objToBeAssigned);

      /**
       * Assignment Operator.
       */
      void operator=(DoubleBuffer& objToBeAssigned);

      /**
       * Output Operator.
       */
      friend std::ostream& operator<<(std::ostream& os, DoubleBuffer& obj);

      /**
       * Destructor.
       */
      virtual ~DoubleBuffer();

      //-----------------------------------------------------------------------
      // Raw methods for access the buffers

      // Get the current read buffer

      void* grabReadBuffer();
      void releaseReadBuffer();

      // Get the current write buffer

      void* grabWriteBuffer();
      void releaseWriteBuffer();

      void switchBuffers();

    protected:

      BufferLock buf1_;
      BufferLock buf2_;

      BufferLock* readBuf_;
      BufferLock* writeBuf_;

    }; // End class DoubleBuffer

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_DOUBLEBUFFER_H
