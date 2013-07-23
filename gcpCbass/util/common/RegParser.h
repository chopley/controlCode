#ifndef GCP_UTIL_REGPARSER_H
#define GCP_UTIL_REGPARSER_H

/**
 * @file RegParser.h
 * 
 * Tagged: Sat Oct  2 20:30:19 UTC 2004
 * 
 * @author 
 */
#include "gcp/util/common/RegDescription.h"

#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"

namespace gcp {
  namespace util {
    
    class RegParser {
    public:
      
      /**
       * Constructor.
       *
       * If archivedOnly = true, byte offset calculations, etc. will
       * be done wrt the archived array map.  If false, they will be
       * done wrt the whole array map.
       */
      RegParser(bool archivedOnly_=false);
      
      /**
       * Destructor.
       */
      virtual ~RegParser();
      
      /**
       * Parse a register block description
       */
      std::vector<RegDescription> inputRegs(std::string regStr,
					    RegInputMode mode=REG_INPUT_RANGE, 
					    bool tell=true,
					    bool extend=true,
					    bool splitIndices=false);
      
      /**
       * Convenience method that uses external arraymap and constructs
       * the stream for an input string.
       */
      std::vector<RegDescription> inputRegs(std::string regStr, 
					    ArrayMap* arrayMap,
					    RegInputMode mode, 
					    bool tell,
					    bool extend,
					    bool splitIndices=false,
					    bool doThrow=true);
      
      /**
       * Parse a register block description.
       *
       * If splitIndices = true, and multiple indices are specified
       * for all but the fastest-changing index, this will return
       * separate descriptors for each index specified.  ie:
       *
       *   receiver.bolometers.ac[0-3][0-99]
       *
       * would return 4 descriptors instead of one.
       *
       */
      std::vector<RegDescription> inputRegs(InputStream* stream, 
					    bool tell,
					    ArrayMap* arraymap,
					    RegInputMode mode, 
					    bool extend,
					    bool splitIndices=false,
					    bool doThrow=true);
      
      /**
       * Parse a single register block description
       */
      RegDescription inputReg(std::string regStr,
			      RegInputMode mode=REG_INPUT_RANGE, 
			      bool tell=true,
			      bool extend=true);
      
      /**
       * Parse a single register block description
       */
      RegDescription inputReg(InputStream* stream, 
			      bool tell,
			      RegInputMode mode, 
			      bool extend,
			      ArrayMap* arraymap=0);
      
      /**
       * Return the validity flag for the last register read
       */
      inline RegValidity validity() {
	return validity_;
      }
      
      /**
       * Return the size in slots of each register element
       */
      static unsigned getSize(RegMapBlock* block, RegAspect aspect, bool extend);
      
    private:
      
      // An array map
      
      ArrayMapBase arrayMap_;
      
      RegValidity validity_; // The validity flag of the last read
                             // register specification
      
      bool archivedOnly_;  // If true, offsets for the archived
                           // register map should be used.
      
      // True if the next token is a '.' followed by some string

      bool wordFollows(InputStream* stream);

      /**
       * Read a register aspect
       */
      RegAspect checkAspect(InputStream* stream);

      /**
       * Read a register integration specifier
       */
      RegInteg checkInteg(InputStream* stream);

      /**
       * Read all index range specifications following a register name
       */
      CoordRange readIndexRanges(InputStream* stream);
      
      /**
       * Read an index range specification
       */
      Range<unsigned> readIndexRange(InputStream* stream);
      
      /**
       * Check that the register specification was compatible with the
       * input mode
       */
      void checkValidityOfMode(RegInputMode mode, ArrRegMap* aregmap,
			       RegMapBoard* brd, RegMapBlock* blk, 
			       CoordRange& range, unsigned size);
      
      
    }; // End class RegParser
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_REGPARSER_H
