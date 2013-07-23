// $Id: NetUnion.h,v 1.1.1.1 2009/07/06 23:57:26 eml Exp $

#ifndef GCP_UTIL_NETUNION_H
#define GCP_UTIL_NETUNION_H

/**
 * @file gcp/util/common/NetDat.h
 * 
 * Tagged: Tue Jun 28 15:51:41 PDT 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:26 $
 * 
 * @author Erik Leitch
 */
#include <map>
#include "gcp/util/common/DataType.h"

#include "gcp/util/common/NetDat.h"

namespace gcp {
  namespace util {

    class NetUnion : public NetDat {
    public:

      static const unsigned NETUNION_UNKNOWN = 0;

      /**
       * Constructor.
       */
      NetUnion();

      /**
       * Conunionor.
       */
      NetUnion(const NetUnion& netUnion);
      NetUnion(NetUnion& netUnion);
      NetUnion& operator=(const NetUnion& netUnion);
      NetUnion& operator=(NetUnion& netUnion);

      /**
       * Destructor.
       */
      virtual ~NetUnion();

      /**
       * Register a member
       */
      void addMember(unsigned id, NetDat* member=0, bool alloc=false);

      /**
       * Add a variable to the internal vector of members
       */
      void addVar(unsigned id, gcp::util::DataType::Type type, 
		  void* vPtr, unsigned nEl);

      /**
       * Add just an id
       */
      void addCase(unsigned id);

      /**
       * Get a member by id
       */
      NetDat* const findMember(unsigned id);

      bool memberIsValid(unsigned id);

      /**
       * Find a member
       */
      NetDat* const getMember(unsigned id);

      /**
       * Set the internal id to the requested member
       */
      void setTo(unsigned id);

      /**
       * Return the message type
       */
      unsigned getType();

      /**
       * Return the size of the member associated with this id
       */
      unsigned sizeOf(unsigned id);

      /**
       * Return the actual size of this object
       */
      unsigned size();

      /**
       * De-serialize data into this struct
       */
      void deserialize(const std::vector<unsigned char>& bytes);
      void deserialize(const unsigned char* bytes);

    private:

      // A map of members contained in this class

      std::map<unsigned int, NetDat::Info> members_;

      // The member currently selected

      unsigned int id_;

      /**
       * Check the size of an array against our size
       */
      void checkSize(const std::vector<unsigned char>& bytes);

      /**
       * Check the size of an array against our size
       */
      void checkSize(const std::vector<unsigned char>& bytes, unsigned id);

      /**
       * Increment the maximum size
       */
      void resize(unsigned size);

      /**
       * Serialize the data in this struct
       */
      void serialize();

    }; // End class NetUnion

  } // End namespace util
} // End namespace gcp

#define NETUNION_UCHAR(id, name) \
addVar(id, gcp::util::DataType::UCHAR, (void*)&name, 1)

#define NETUNION_UCHAR_ARR(id, name, nEl) \
addVar(id, gcp::util::DataType::UCHAR, (void*)name, nEl)

#define NETUNION_CHAR(id, name) \
addVar(id, gcp::util::DataType::CHAR, (void*)&name, 1)

#define NETUNION_CHAR_ARR(id, name, nEl) \
addVar(id, gcp::util::DataType::CHAR, (void*)name, nEl)

#define NETUNION_BOOL(id, name) \
addVar(id, gcp::util::DataType::BOOL, (void*)&name, 1)

#define NETUNION_BOOL_ARR(id, name, nEl) \
addVar(id, gcp::util::DataType::BOOL, (void*)name, nEl)

#define NETUNION_USHORT(id, name) \
addVar(id, gcp::util::DataType::USHORT, (void*)&name, 1)

#define NETUNION_USHORT_ARR(id, name, nEl) \
addVar(id, gcp::util::DataType::USHORT, (void*)name, nEl)

#define NETUNION_SHORT(id, name) \
addVar(id, gcp::util::DataType::SHORT, (void*)&name, 1)

#define NETUNION_SHORT_ARR(id, name, nEl) \
addVar(id, gcp::util::DataType::SHORT, (void*)name, nEl)

#define NETUNION_UINT(id, name) \
addVar(id, gcp::util::DataType::UINT, (void*)&name, 1)

#define NETUNION_UINT_ARR(id, name, nEl) \
addVar(id, gcp::util::DataType::UINT, (void*)name, nEl)

#define NETUNION_INT(id, name) \
addVar(id, gcp::util::DataType::INT, (void*)&name, 1)

#define NETUNION_INT_ARR(id, name, nEl) \
addVar(id, gcp::util::DataType::INT, (void*)name, nEl)

#define NETUNION_ULONG(id, name) \
addVar(id, gcp::util::DataType::ULONG, (void*)&name, 1)

#define NETUNION_ULONG_ARR(id, name, nEl) \
addVar(id, gcp::util::DataType::ULONG, (void*)name, nEl)

#define NETUNION_LONG(id, name) \
addVar(id, gcp::util::DataType::LONG, (void*)&name, 1)

#define NETUNION_LONG_ARR(id, name, nEl) \
addVar(id, gcp::util::DataType::LONG, (void*)name, nEl)

#define NETUNION_FLOAT(id, name) \
addVar(id, gcp::util::DataType::FLOAT, (void*)&name, 1)

#define NETUNION_FLOAT_ARR(id, name, nEl) \
addVar(id, gcp::util::DataType::FLOAT, (void*)name, nEl)

#define NETUNION_DOUBLE(id, name) \
addVar(id, gcp::util::DataType::DOUBLE, (void*)&name, 1)

#define NETUNION_DOUBLE_ARR(id, name, nEl) \
addVar(id, gcp::util::DataType::DOUBLE, (void*)name, nEl)

#endif // End #ifndef GCP_UTIL_NETUNION_H
