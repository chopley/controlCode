#ifndef GCP_UTIL_ARCHIVERWRITERDIRFILE_H
#define GCP_UTIL_ARCHIVERWRITERDIRFILE_H

/**
 * @file ArchiverWriterDirfile.h
 * 
 * Tagged: Sat 28-Jan-06 07:25:34
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/ArchiverWriter.h"

#include <list>
#include <string>
#include <vector>

#include <unistd.h>

class ArrayMap;
class ArrRegMap;
class RegMapBlock;

namespace gcp {
  namespace util {
    
    class ArchiverWriterDirfile : public ArchiverWriter {
    public:
      
      // ArchiverWriterDirfil will maintain an array of objects of the
      // following type, one per archived register. 

      struct Register {

	// The name of the file in which this register will be written

	std::string name_;   

	// A pointer to the beginning of the memory for this register
	// value in the Archiver frame buffer

	void* ptr_;

	// The byte offset into the frame of this register

	unsigned byteOffset_;

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

	// A format string for this register

	std::string format_;

	// A pointer to the descriptor for this register

	RegMapBlock* block_;

	// A buffer for data

	std::vector<unsigned char> bytes_;

	// Constructors

	Register(RegMapBlock* block, 
		 ArrRegMap* arrRegMap, 
		 unsigned startEl,
		 unsigned nEl,
		 unsigned char* base,
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
      };

      /**
       * Constructor.
       */
      ArchiverWriterDirfile(unsigned nBuffer=1);
      
      void initialize(ArrayMap* arrayMap, unsigned char* basePtr, 
		      unsigned nBuffer);

      void mapRegisters(ArrayMap* arrayMap, 
			unsigned char* basePtr, 
			unsigned nBuffer);

      /**
       * Copy Constructor.
       */
      ArchiverWriterDirfile(const ArchiverWriterDirfile& objToBeCopied);
      
      /**
       * Copy Constructor.
       */
      ArchiverWriterDirfile(ArchiverWriterDirfile& objToBeCopied);
      
      /**
       * Const Assignment Operator.
       */
      void operator=(const ArchiverWriterDirfile& objToBeAssigned);
      
      /**
       * Assignment Operator.
       */
      void operator=(ArchiverWriterDirfile& objToBeAssigned);
      
      /**
       * Destructor.
       */
      virtual ~ArchiverWriterDirfile();

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

      // True if archive files are currently open

      bool isOpen_;

      // The list of registers

      std::list<Register> registers_;

      // The base directory in which we are working

      std::string dirname_;

      // Insert a register

      void insert(RegMapBlock* block, 
		  ArrRegMap* arrRegMap, 
		  unsigned char* basePtr,
		  unsigned nBuffer);

      // Insert a block of register elements into the list
      
      void insertReg(RegMapBlock* block, 
		     ArrRegMap* arrRegMap, 
		     unsigned startEl,
		     unsigned nEl,
		     unsigned char* basePtr,
		     bool writeIndex,
		     unsigned nBuffer);

      // Create the directory for the current set of archive files

      int createDir(char* dir);

      // Write the format file needed for things like KST

      int outputFormatFile(char* dir);

    }; // End class ArchiverWriterDirfile
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_ARCHIVERWRITERDIRFILE_H
