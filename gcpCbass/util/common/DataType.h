// $Id: DataType.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_DATATYPE_H
#define GCP_UTIL_DATATYPE_H

/**
 * @file DataType.h
 * 
 * Tagged: Tue Jun 22 22:32:16 UTC 2004
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author 
 */
#include "gcp/control/code/unix/libunix_src/common/regmap.h"

#include "gcp/util/common/Complex.h"
#include "gcp/util/common/RegDate.h"
#include <ostream>

namespace gcp {
  namespace util {
    
    /**
     * Enumerate data types
     */
    class DataType {
    public:
      
      enum Type {
	NONE          = 0x0,
	UNKNOWN       = 0x0,
	UCHAR         = 0x1,
	CHAR          = 0x2,
	BOOL          = 0x4,
	USHORT        = 0x8,
	SHORT         = 0x10,
	UINT          = 0x20,
	INT           = 0x40,
	ULONG         = 0x80,
	LONG          = 0x100,
	FLOAT         = 0x200,
	DOUBLE        = 0x400,
	DATE          = 0x800,
	COMPLEX_FLOAT = 0x1000,
	STRING        = 0x2000
      };

      
      DataType();
      DataType(bool b);
      DataType(unsigned char uc);
      DataType(char ch);
      DataType(unsigned short us);
      DataType(short s);
      DataType(unsigned int ui);
      DataType(int i);
      DataType(unsigned long ul);
      DataType(long l);
      DataType(float f);
      DataType(double d);
      DataType(gcp::util::RegDate date);
      DataType(gcp::util::Complex<float> cf);
      
      virtual ~DataType();
      
      /**
       * Return the size, in bytes, of the requested type
       */
      static unsigned sizeOf(Type type);
      
      /**
       * Return the size, in bytes, of the type given in RegFlags
       * specification
       */
      static unsigned sizeOf(RegMapBlock* blk);
      
      /**
       * Return the data type of a block
       */
      static Type typeOf(RegMapBlock* blk);
      
      // Take the stored value and convert to the type corresponding
      // to a register block

      void convertToTypeOf(RegMapBlock* blk);
      double getValAsDouble();

      /**
       * Assignment operators
       */
      void operator=(bool b);
      void operator=(unsigned char uc);
      void operator=(char ch);
      void operator=(unsigned short us);
      void operator=(short s);
      void operator=(unsigned int ui);
      void operator=(int i);
      void operator=(unsigned long ul);
      void operator=(long l);
      void operator=(float f);
      void operator=(double d);
      void operator=(gcp::util::RegDate date);
      void operator=(gcp::util::Complex<float> cf);
      
      /**
       * Assignment operators for pointers
       */
      void operator=(bool* b);
      void operator=(unsigned char* uc);
      void operator=(char* ch);
      void operator=(unsigned short* us);
      void operator=(short* s);
      void operator=(unsigned int* ui);
      void operator=(int* i);
      void operator=(unsigned long* ul);
      void operator=(long* l);
      void operator=(float* f);
      void operator=(double* d);
      void operator=(gcp::util::RegDate* date);
      void operator=(gcp::util::Complex<float>* cf);

      void operator=(DataType& dataType);
      void operator=(const DataType& dataType);
      
      void operator-=(DataType& dataType);

      bool operator==(DataType& dataType);
      bool operator>(DataType& dataType);
      bool operator>=(DataType& dataType);
      bool operator<(DataType& dataType);
      bool operator<=(DataType& dataType);

      void operator++();

      void convertToAbs();

      friend std::ostream& operator<<(std::ostream& os, DataType& dataType);
      friend std::ostream& operator<<(std::ostream& os, DataType::Type& type);

      /**                                                                                                           
       * Return a void ptr to the data for this data type
       */
      void* data();

      // The actual data in this DataType will be stored as a union

      union {
	bool b;
	unsigned char uc;
	char c;
	unsigned short us;
	short s;
	unsigned int ui;
	int i;
	unsigned long ul;
	long l;
	float f;
	double d;
	gcp::util::RegDate::Data date;
	gcp::util::Complex<float>::Data cf;
      } data_;
      
      Type type_;
      
      void checkType(DataType& dataType);

      // True if we are indexing an array of data

      bool isArray_;

      // A void ptr to the array

      void* ptr_;

      // Overloaded utility methods

      static Type typeOf(bool* obj);
      static Type typeOf(unsigned char* obj);
      static Type typeOf(char* obj);
      static Type typeOf(unsigned short* obj);
      static Type typeOf(short* obj);
      static Type typeOf(unsigned int* obj);
      static Type typeOf(int* obj);
      static Type typeOf(unsigned long* obj);
      static Type typeOf(long* obj);
      static Type typeOf(float* obj);
      static Type typeOf(double* obj);
      static Type typeOf(gcp::util::RegDate* obj);
      static Type typeOf(gcp::util::Complex<float>* obj);
      static Type typeOf(gcp::util::RegDate::Data* obj);
      static Type typeOf(gcp::util::Complex<float>::Data* obj);

    }; // End class DataType
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_DATATYPE_H
