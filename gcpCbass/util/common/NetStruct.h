// $Id: NetStruct.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_NETSTRUCT_H
#define GCP_UTIL_NETSTRUCT_H

/**
 * @file gcp/util/common/NetDat.h
 * 
 * Tagged: Wed Jul  6 15:09:03 PDT 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/NetDat.h"
#include "gcp/util/common/DataType.h"

namespace gcp {
  namespace util {

    class NetStruct : public NetDat {
    public:

      /**
       * Constructor.
       */
      NetStruct();

      /**
       * Constructor.
       */
      NetStruct(const NetStruct& netStruct);
      NetStruct(NetStruct& netStruct);
      void operator=(const NetStruct& netStruct);
      void operator=(NetStruct& netStruct);

      /**
       * Destructor.
       */
      virtual ~NetStruct();

      /**
       * Add a member to the internal vector of members
       */
      virtual void addMember(NetDat* netDat, bool alloc=false);

      /**
       * Add a variable to the internal vector of members
       */
      void addVar(gcp::util::DataType::Type type, void* vPtr, unsigned nEl);

      /**
       * De-serialize data into this struct
       */
      void deserialize(const std::vector<unsigned char>& bytes);
      void deserialize(const unsigned char* bytes);

      /**
       * Return the size of this object
       */
      virtual unsigned size();

    private:

      // A vector of objects contained in this class

      std::vector<NetDat::Info> members_;

      /**
       * Serialize the data in this struct
       */
      void serialize();

      /**
       * Check the size of an array against our size
       */
      void checkSize(const std::vector<unsigned char>& bytes);

    }; // End class NetStruct

  } // End namespace util
} // End namespace gcp


#define NETSTRUCT_UCHAR(name) \
addVar(gcp::util::DataType::UCHAR, (void*)&name, 1)

#define NETSTRUCT_UCHAR_ARR(name, nEl) \
addVar(gcp::util::DataType::UCHAR, (void*)name, nEl)

#define NETSTRUCT_UCHAR_VEC(name) \
addVar(gcp::util::DataType::UCHAR, (void*)&name[0], name.size())

#define NETSTRUCT_CHAR(name) \
addVar(gcp::util::DataType::CHAR, (void*)&name, 1)

#define NETSTRUCT_CHAR_ARR(name, nEl) \
addVar(gcp::util::DataType::CHAR, (void*)name, nEl)

#define NETSTRUCT_CHAR_VEC(name) \
addVar(gcp::util::DataType::CHAR, (void*)&name[0], name.size())

#define NETSTRUCT_BOOL(name) \
addVar(gcp::util::DataType::BOOL, (void*)&name, 1)

#define NETSTRUCT_BOOL_ARR(name, nEl) \
addVar(gcp::util::DataType::BOOL, (void*)name, nEl)

#define NETSTRUCT_BOOL_VEC(name) \
addVar(gcp::util::DataType::BOOL, (void*)&name[0], name.size())

#define NETSTRUCT_USHORT(name) \
addVar(gcp::util::DataType::USHORT, (void*)&name, 1)

#define NETSTRUCT_USHORT_ARR(name, nEl) \
addVar(gcp::util::DataType::USHORT, (void*)name, nEl)

#define NETSTRUCT_USHORT_VEC(name) \
addVar(gcp::util::DataType::USHORT, (void*)&name[0], name.size())

#define NETSTRUCT_SHORT(name) \
addVar(gcp::util::DataType::SHORT, (void*)&name, 1)

#define NETSTRUCT_SHORT_ARR(name, nEl) \
addVar(gcp::util::DataType::SHORT, (void*)name, nEl)

#define NETSTRUCT_SHORT_VEC(name) \
addVar(gcp::util::DataType::SHORT, (void*)&name[0], name.size())

#define NETSTRUCT_UINT(name) \
addVar(gcp::util::DataType::UINT, (void*)&name, 1)

#define NETSTRUCT_UINT_ARR(name, nEl) \
addVar(gcp::util::DataType::UINT, (void*)name, nEl)

#define NETSTRUCT_UINT_VEC(name) \
addVar(gcp::util::DataType::UINT, (void*)&name[0], name.size())

#define NETSTRUCT_INT(name) \
addVar(gcp::util::DataType::INT, (void*)&name, 1)

#define NETSTRUCT_INT_ARR(name, nEl) \
addVar(gcp::util::DataType::INT, (void*)name, nEl)

#define NETSTRUCT_INT_VEC(name) \
addVar(gcp::util::DataType::INT, (void*)&name[0], name.size())

#define NETSTRUCT_ULONG(name) \
addVar(gcp::util::DataType::ULONG, (void*)&name, 1)

#define NETSTRUCT_ULONG_ARR(name, nEl) \
addVar(gcp::util::DataType::ULONG, (void*)name, nEl)

#define NETSTRUCT_ULONG_VEC(name) \
addVar(gcp::util::DataType::ULONG, (void*)&name[0], name.size())

#define NETSTRUCT_LONG(name) \
addVar(gcp::util::DataType::LONG, (void*)&name, 1)

#define NETSTRUCT_LONG_ARR(name, nEl) \
addVar(gcp::util::DataType::LONG, (void*)name, nEl)

#define NETSTRUCT_LONG_VEC(name) \
addVar(gcp::util::DataType::LONG, (void*)&name[0], name.size())

#define NETSTRUCT_FLOAT(name) \
addVar(gcp::util::DataType::FLOAT, (void*)&name, 1)

#define NETSTRUCT_FLOAT_ARR(name, nEl) \
addVar(gcp::util::DataType::FLOAT, (void*)name, nEl)

#define NETSTRUCT_FLOAT_VEC(name) \
addVar(gcp::util::DataType::FLOAT, (void*)&name[0], name.size())

#define NETSTRUCT_DOUBLE(name) \
addVar(gcp::util::DataType::DOUBLE, (void*)&name, 1)

#define NETSTRUCT_DOUBLE_ARR(name, nEl) \
addVar(gcp::util::DataType::DOUBLE, (void*)name, nEl)

#define NETSTRUCT_DOUBLE_VEC(name) \
addVar(gcp::util::DataType::DOUBLE, (void*)&name[0], name.size())

#define NETSTRUCT_STRING(name) \
addVar(gcp::util::DataType::UCHAR, (void*)&name[0], name.size())

#endif // End #ifndef GCP_UTIL_NETSTRUCT_H
