#ifndef ANTENNAEXCEPTION_H
#define ANTENNAEXCEPTION_H

/**
 * @file AntennaException.h
 * 
 * Started: Wed Feb 25 15:06:20 PST 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Exception.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      class AntennaException : gcp::util::Exception {
      public:
	
	enum Type {
	  ACU,
	  UNKNOWN,
	};
	
	/**
	 * Construct an AntennaException with a detailed message.
	 *
	 * @param str String describing error.
	 * @param fileName where exception originated.
	 * @param lineNumber exception occurred on.
	 */
	inline AntennaException(std::string str, const char * fileName,  
				const int lineNumber, Type type) :
	  gcp::util::Exception(str, fileName, lineNumber, true), type_(type) {}
	
	/**
	 * Construct an Error with a detailed message.
	 * 
	 * @param os ostringstream containing message
	 * @param fileName where exception originated.
	 * @param lineNumber exception occurred on.
	 */
	inline AntennaException(std::ostringstream& os, const char * fileName, 
				const int lineNumber, Type type) :
	  gcp::util::Exception(os, fileName, lineNumber, true), type_(type) {}
	
	/** 
	 * Constructor with an LogStream&.
	 */
	inline AntennaException(gcp::util::LogStream& ls, 
				const char* fileName, const int lineNumber,
				Type type) :
	  gcp::util::Exception(ls, fileName, lineNumber, true), type_(type) {}
	
	/** 
	 * Constructor with an LogStream*.
	 */
	inline AntennaException(gcp::util::LogStream* ls, 
				const char* fileName, const int lineNumber,
				Type type) :
	  gcp::util::Exception(ls, fileName, lineNumber, true), type_(type) {}
	
	/**
	 * Set the type of error.
	 */
	inline void setAcu() {type_ = ACU;}

	/**
	 * Query the type of error.
	 */
	inline bool isAcu() {return type_ == ACU;}
	
      private:
	
	Type type_;
	
      }; // End class AntennaException
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#define AntError(x) AntennaException((x), __FILE__, __LINE__, \
AntennaException::UNKNOWN)

#define AntErrorDef(x,y) AntennaException (x)((y), __FILE__, __LINE__, \
AntennaException::UNKNOWN))

#define AcuError(x) AntennaException((x), __FILE__, __LINE__, \
AntennaException::ACU)

#define AcuErrorDef(x,y) AntennaException (x)((y), __FILE__, __LINE__, \
AntennaException::ACU))

#endif // End #ifndef 


