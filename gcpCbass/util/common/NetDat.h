// $Id: NetDat.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_NETDAT_H
#define GCP_UTIL_NETDAT_H

/**
 * @file gcp/util/common/NetDat.h
 * 
 * Tagged: Wed Jul  6 13:41:09 PDT 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author Erik Leitch
 */
#include <vector>

namespace gcp {
  namespace util {

    class NetStruct;
    class NetUnion;

    class NetDat {
    public:

      struct Info {
	NetDat* datPtr_;
	bool alloc_;

	Info() {
	  datPtr_ = 0;
	  alloc_ = false;
	};

	Info(NetDat* datPtr, bool alloc) {
	  datPtr_ = datPtr;
	  alloc_  = alloc;
	};

	Info(const Info& info) {
	  datPtr_ = info.datPtr_;
	  alloc_  = info.alloc_;
	};
      };

      /**
       * Constructor.
       */
      NetDat();

      /**
       * Constructor.
       */
      NetDat(const NetDat& netDat);
      NetDat(NetDat& netDat);
      void operator=(const NetDat& netDat);
      void operator=(NetDat& netDat);

      /**
       * Destructor.
       */
      virtual ~NetDat();

      /**
       * Return size in bytes
       */
      virtual unsigned size();

      /**
       * Return a reference to serialized data
       */
      virtual std::vector<unsigned char>& getSerializedData();

      /**
       * Fill an external pointer with serialized data
       */
      virtual void packSerializedData(unsigned char* bytes);

      /**
       * Deserialize data into this object
       */
      virtual void deserialize(const std::vector<unsigned char>& bytes);
      
      /**
       * Return the maximum size (for variable sized objects) in bytes
       */
      unsigned maxSize();

      /**
       * Send this object over a socket
       */
      bool send(int fd);

      /**
       * Receive this object over a socket
       */
      bool recv(int fd);

    protected:

      friend class NetStruct;
      friend class NetUnion;

      /**
       * Memory for the serialized version of this data
       */
      std::vector<unsigned char> bytes_;

      /**
       * Maximum size in bytes
       */
      unsigned maxSize_;

      /**
       * Increment size
       */
      virtual void resize(unsigned size);

      /**
       * Return a reference to internal data
       */
      unsigned char* const getSerializedDataPtr();

      /**
       * Serialize the data
       */
      virtual void serialize();

      /**
       * Private deserialization method
       */
      virtual void deserialize(const unsigned char* bytes)=0;

      /**
       * Check an array size against ours
       */
      virtual void checkSize(const std::vector<unsigned char>& bytes);

    }; // End class NetDat

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_NETDAT_H
