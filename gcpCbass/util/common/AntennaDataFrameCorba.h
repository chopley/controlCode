#ifndef GCP_UTIL_ANTENNADATAFRAMECORBA_H
#define GCP_UTIL_ANTENNADATAFRAMECORBA_H

/**
 * @file AntennaDataFrameCorba.h
 * 
 * Tagged: Sun Mar 21 17:36:14 UTC 2004
 * 
 * @author Erik Leitch
 */
#include <vector>

#include "gcp/util/common/Directives.h"

#if DIR_USE_ANT_CORBA
#include <OB/CORBA.h>
#endif

#include "gcp/util/common/DataFrame.h"
#include "gcp/util/common/AntennaDataFrame.h"

#if DIR_USE_ANT_CORBA
#include "gcp/antenna/corba/DataFrame.h"
#endif

namespace gcp {
  namespace util {
    
    class AntennaDataFrameCorba : 
      public gcp::util::AntennaDataFrame,
      public gcp::util::DataFrame {
      
      public:
      
      /**
       * Constructors.
       */
      AntennaDataFrameCorba();
      
      /**
       * Constructor with buffer initialization
       */
      AntennaDataFrameCorba(unsigned int nBuffer);
      
#if DIR_USE_ANT_CORBA
      /**
       * Constructor with initialization from a DataFrame
       */
      AntennaDataFrameCorba(const gcp::antenna::corba::DataFrame* frame);
#endif
      
      /**
       * Resize the internal buffer.
       */
      void resize(unsigned int nBuffer);
      
      /**
       * Return the size of the internal buffer.
       */
      unsigned int size();
      
#if DIR_USE_ANT_CORBA
      /**
       * Return a reference to a requested element.
       */
      inline unsigned char& operator[](unsigned index)
	{
	  return (unsigned char&)frame_.lvals_[index];
	}
#endif
      
      /**
       * Override the base class setAnt() methods
       */
      void setAnt(gcp::util::AntNum::Id antennaId);
      void setAnt(unsigned int);
      void setAnt(const gcp::util::AntNum& antNum);
      
#if DIR_USE_ANT_CORBA
      /**
       * Assignment operators
       */
      void operator=(DataFrame& frame);
      void operator=(AntennaDataFrameCorba& frame);
#endif
      
      /**
       * Destructor.
       */
      virtual ~AntennaDataFrameCorba();
      
#if DIR_USE_ANT_CORBA      
      /**
       * Return a pointer to the raw data object managed by this
       * class.
       */
      gcp::antenna::corba::DataFrame* frame();
#endif
      /**
       * Get a pointer to our internal data suitable for using as an
       * external network buffer
       */
      unsigned char* data();
      
      private:
      
#if DIR_USE_ANT_CORBA
      /**
       * The underlying CORBA object this object is based on
       */
      gcp::antenna::corba::DataFrame frame_;
#endif
      
      /**
       * An array to shadow frame_.lvals
       */
      std::vector<unsigned char> lvals_;
      
    }; // End class AntennaDataFrameCorba
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_ANTENNADATAFRAMECORBA_H
