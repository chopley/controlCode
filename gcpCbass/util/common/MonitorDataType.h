#ifndef GCP_UTIL_MONITORDATATYPE_H
#define GCP_UTIL_MONITORDATATYPE_H

/**
 * @file MonitorDataType.h
 * 
 * Tagged: Thu Mar 31 00:12:30 PST 2005
 * 
 * @author GCP data acquisition
 */
#include "gcp/util/common/DataType.h"

namespace gcp {
  namespace util {
    
    class MonitorDataType {
    public:
      
      // Enumerate valid formats
      
      enum FormatType {
	FM_CHAR,
	FM_UCHAR,
	FM_BOOL,
	FM_DOUBLE,
	FM_FLOAT,
	FM_SHORT,
	FM_USHORT,
	FM_INT,
	FM_UINT,
	FM_LONG,
	FM_ULONG,
	FM_DATE,
	FM_COMPLEX_FLOAT,
	FM_STRING,
	FM_DEFAULT = FM_UINT,
	FM_UNKNOWN
      };
      
      FormatType nativeFormat;
      FormatType selectedFormat;
      RegAspect aspect;
      
      gcp::util::DataType val;
      std::string stringVal;
      std::string formatStr;
      
      void print();
      
    }; // End class MonitorDataType
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_MONITORDATATYPE_H
