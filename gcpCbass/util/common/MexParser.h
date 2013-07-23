#ifndef GCP_MATLAB_MEXPARSER_H
#define GCP_MATLAB_MEXPARSER_H

/**
 * @file MexParser.h
 * 
 * Tagged: Wed May  4 23:26:43 PDT 2005
 * 
 * @author GCP data acquisition
 */
#include "mex.h"
#include "matrix.h"

namespace gcp {
  namespace util {
    
    class MexParser {
    public:
      
      /**
       * Constructor.
       */
      MexParser(const mxArray*);
      
      /**
       * Destructor.
       */
      virtual ~MexParser();
      
      // Return the dimensionality of an mxArray
      
      int* getDimensions();
      
      int getNumberOfDimensions();
      
      void printDimensions();
      
    private:
      
      mxArray* array_;
      
    }; // End class MexParser
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_MATLAB_MEXPARSER_H
