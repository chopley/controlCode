#ifndef GCP_UTIL_DIRFILEWRITER_H
#define GCP_UTIL_DIRFILEWRITER_H

/**
 * @file DirfileWriter.h
 * 
 * Tagged: Sat 28-Jan-06 07:25:34
 * 
 * @author Erik Leitch
 */
#include "DirfileWriter.h"

#include <list>
#include <string>
#include <vector>

#include <unistd.h>

namespace gcp {
  namespace util {
    
    class DirfileException {
    public:

      DirfileException(std::string message) {
	message_ = message;
      }

      std::string message_;
    };

    class DirfileWriter {
    public:
      
      // ArchiverWriterDirfil will maintain an array of objects of the
      // following type, one per archived register. 
      
      struct Register {
	
	enum Type {
	  NONE          = 0x0,
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
	  COMPLEX       = 0x800
	};
	
	// The name of the file in which this register will be written
	
	std::string name_;   
	
	// A pointer to the beginning of the memory for this register
	// value
	
	void* ptr_;
	
	// The number of bytes in this register block
	
	unsigned nByte_;
	unsigned nBytePerEl_;
	
	// The number of elements in this register block
	
	unsigned nEl_;
	
	// A file descriptor associated with the currently open file
	// corresponding to this register
	
	int fd_;
	
	// The number of elements we will buffer before writing
	
	unsigned nBuffer_;
	
	// The current sample
	
	unsigned iBuffer_;
	
	// The type of this register

	Type type_;

	// A format string for this register
	
	std::string format_;
	
	// A buffer for data
	
	std::vector<unsigned char> bytes_;
	
	// Constructors
	
	Register(std::string name,
		 unsigned char* base,
		 unsigned startEl,
		 unsigned nEl,
		 Type type,
		 bool writeIndex,
		 unsigned nBuffer);
	
	Register(const Register& reg);
	
	virtual ~Register();
	
	// Open a file for this register in the specified directory
	
	int open(char* dir);
	
	// Close the file associated with this register
	
	void close();
	
	// Flush the file associated with this register
	
	void flush();
	
	// Write the current value of this register
	
	int write();
	
	// A format string for this register
	
	std::string format();

	// Return the size, in bytes, of the requested type

	unsigned sizeOf(Type type);

      };
      
      /**
       * Constructor.  
       *
       * nBuffer specifies the number of data values to be buffered
       * for each register before writing to disk
       *
       * writeIndex specfies whether or not to write separatae indices
       * of multi-dimensional registers as separate files, ie:
       *
       *     bolo.channel[0-1] 
       *
       * would be written as bolo.channel0 and bolo.channel1
       */
      DirfileWriter(unsigned nBuffer=1, bool writeIndex=true);
      
      void initialize(unsigned nBuffer, bool writeIndex);
      
      // Add an arbitrary type of register to the list of registers
      // maintained by this object
      
      void addRegister(std::string name, unsigned char* base, Register::Type, 
		       unsigned startEl=0, unsigned nEl=1);
      
      // Add a floating-point register to the list of registers this
      // object will maintain
      
      void addFloatRegister(std::string name, unsigned char* base, unsigned startEl=0, unsigned nEl=1);

      /**
       * Copy Constructor.
       */
      DirfileWriter(const DirfileWriter& objToBeCopied);
      
      /**
       * Copy Constructor.
       */
      DirfileWriter(DirfileWriter& objToBeCopied);
      
      /**
       * Const Assignment Operator.
       */
      void operator=(const DirfileWriter& objToBeAssigned);
      
      /**
       * Assignment Operator.
       */
      void operator=(DirfileWriter& objToBeAssigned);
      
      /**
       * Destructor.
       */
      virtual ~DirfileWriter();
      
      // Overloaded methods of the base class that will be called by
      // the Archiver parent
      
      int openArcfile(std::string dir);
      void closeArcfile();
      void flushArcfile();
      int writeIntegration();
      bool isOpen();
      
    protected:
      
      // This is the number of samples we will buffer before writing to disk
      
      unsigned nBuffer_;

      unsigned maxlen_;

      bool writeIndex_;

      // True if archive files are currently open
      
      bool isOpen_;
      
      // The list of registers
      
      std::list<Register> registers_;
      
      // The base directory in which we are working
      
      std::string dirname_;
      
      // Insert a block of register elements into the list
      
      void insertReg(std::string name,
		     unsigned char* basePtr,
		     unsigned startEl,
		     unsigned nEl,
		     Register::Type type);
      
      // Create the directory for the current set of archive files
      
      int createDir(char* dir);
      
      // Write the format file needed for things like KST
      
      int outputFormatFile(char* dir);
      
    }; // End class DirfileWriter   
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_DIRFILEWRITER_H
