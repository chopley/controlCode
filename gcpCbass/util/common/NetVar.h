// $Id: NetVar.h,v 1.2 2009/08/17 21:18:31 eml Exp $

#ifndef GCP_UTIL_NETVAR_H
#define GCP_UTIL_NETVAR_H

/**
 * @file gcp/util/common/NetDat.h
 * 
 * Tagged: Wed Jul  6 13:31:47 PDT 2005
 * 
 * @version: $Revision: 1.2 $, $Date: 2009/08/17 21:18:31 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/DataType.h"

#include "gcp/util/common/NetDat.h"

namespace gcp {
  namespace util {

    class NetStruct;

    class NetVar : public NetDat {
    public:

      /**
       * Constructor.
       */
      NetVar(gcp::util::DataType::Type type, void* vPtr, unsigned nEl);

      /**
       * Destructor.
       */
      virtual ~NetVar();

      /**
       * Copy constructor
       */
      NetVar(const NetVar& netVar);
      NetVar(NetVar& netVar);

      /**
       * Deserialize data into this object
       */
      void deserialize(const std::vector<unsigned char>& bytes);

      unsigned size();

      // The type of this variable

      gcp::util::DataType::Type type_;

    private:

      // Pointer to where the start of the memory resides for this
      // variable

      void* vPtr_;

      // Number of elements in this variable

      unsigned nEl_;

      /**
       * Size of this variable
       */
      unsigned size_;

      /**
       * Serialize the data for this variable
       */
      void serialize();

      /**
       * Deserialize data into this object
       */
      void deserialize(const unsigned char* bytes);

      /**
       * Resize
       */
      void resize(unsigned size);

    }; // End class NetVar

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_NETVAR_H
