#ifndef GCP_UTIL_DATAFRAMENORMAL_H
#define GCP_UTIL_DATAFRAMENORMAL_H

/**
 * @file DataFrameNormal.h
 * 
 * Tagged: Sun Mar 21 15:15:45 UTC 2004
 * 
 * @author Erik Leitch
 */
#include <vector>

#include "gcp/util/common/DataFrame.h"

namespace gcp {
  namespace util {
    
    class DataFrameNormal : public DataFrame {
    public:
      
      /**
       * Constructors.
       */
      DataFrameNormal();
      
      /**
       * Constructor with buffer initialization
       */
      DataFrameNormal(unsigned int nBuffer);
      
      /**
       * Resize the internal buffer.
       */
      void resize(unsigned int nBuffer);
      
      /**
       * Return the size of the internal buffer.
       */
      unsigned int size();
      
      /**
       * Return a reference to a requested element.
       */
      inline unsigned char& operator[](unsigned index)
	{
	  return lvals_[index];
	}
      
      /**
       * Assignment operators, base-class and local
       */
      virtual void operator=(DataFrame& frame);
      virtual void operator=(DataFrameNormal& frame);
      
      /**
       * Get a pointer to our internal data suitable for using as an
       * external network buffer
       */
      virtual unsigned char* data();
      
      /**
       * Destructor.
       */
      virtual ~DataFrameNormal();
      
    private:
      
      std::vector<unsigned char> lvals_;
      
    }; // End class DataFrameNormal
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_DATAFRAMENORMAL_H
