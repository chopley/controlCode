// $Id: IdlParser.h,v 1.1.1.1 2009/07/06 23:57:22 eml Exp $

#ifndef GCP_IDL_IDLPARSER_H
#define GCP_IDL_IDLPARSER_H

/**
 * @file IdlParser.h
 * 
 * Tagged: Thu Jul 19 14:46:32 PDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:22 $
 * 
 * @author username: Command not found.
 */

#include <iostream>
#include <vector>

#include "idl_export.h"

#include "gcp/util/common/Exception.h"

namespace gcp {
  namespace idl {

    class IdlParser {
    public:

      struct IdlStructTag {
	std::string name_;
	bool isStruct_;
	bool isArray_;
	std::vector<unsigned> dims_;
      };

      /**
       * Constructor.
       */
      IdlParser();
      IdlParser(IDL_VPTR vPtr);
      
      void setTo(IDL_VPTR vPtr);

      void printSomething() {
	std::cout << "Hello from IdlParser" << std::endl;
      }

      /**
       * Destructor.
       */
      virtual ~IdlParser();

      // Return the dimensionality of a variable
      
      std::vector<unsigned> getDimensions();
      
      unsigned getDimension(unsigned iDim);
      int getNumberOfDimensions();
      void printDimensions();
      
      // Return pointers to appropriate data types
      
      double* getDoubleData();
      
      bool isStruct();
      bool isArray();
      bool isDouble();
      bool isString();

      //------------------------------------------------------------
      // String handling methods
      //------------------------------------------------------------

      std::string getString(unsigned index=0);

      //------------------------------------------------------------
      // Array handling methods
      //------------------------------------------------------------

      unsigned getNumberOfElements();
      unsigned getDataSizeOfElement();
      unsigned getSizeOf(unsigned idlFlag);

      //------------------------------------------------------------
      // Struct handling methods
      //------------------------------------------------------------

      unsigned getNumberOfTags();
      std::vector<IdlParser> getTagList();
      IdlParser getTag(std::string tagName);
      unsigned dataOffsetOfTag(std::string tagName);
      char* getPtrToDataForTag(std::string tagName);

      std::string name() {
	return name_;
      }

      //------------------------------------------------------------
      // General utility
      //------------------------------------------------------------

      char* getPtrToData();

    private:

      bool isPrimary_;
      std::string name_;
      IDL_VPTR vPtr_;
      char* dataBase_;

      char* getBaseDataPtr();

    }; // End class IdlParser

  } // End namespace idl
} // End namespace gcp



#endif // End #ifndef GCP_IDL_IDLPARSER_H
