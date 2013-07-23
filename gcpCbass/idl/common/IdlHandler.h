// $Id: IdlHandler.h,v 1.1.1.1 2009/07/06 23:57:22 eml Exp $

#ifndef GCP_IDL_IDLHANDLER_H
#define GCP_IDL_IDLHANDLER_H

/**
 * @file IdlHandler.h
 * 
 * Tagged: Wed Jul 18 23:50:23 PDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:22 $
 * 
 * @author username: Command not found.
 */

#include <iostream>
#include <list>
#include <vector>

#include "idl_export.h"

#include "gcp/util/common/DataType.h"
#include "gcp/util/common/MonitorDataType.h"

namespace gcp {
  namespace idl {

    //------------------------------------------------------------
    // A class for managing structure definitions
    //------------------------------------------------------------

    class IdlStructDef {
    public:

      struct IdlStructTag {
	std::string name_;
	std::vector<unsigned> dims_;
	void* type_;
	IdlStructDef* def_;

	IdlStructTag(const IdlStructTag& tag) {
	  *this = (IdlStructTag&) tag;
	}

	IdlStructTag(IdlStructTag& tag) {
	  *this = (IdlStructTag&) tag;
	}

	void operator=(const IdlStructTag& tag) {
	  *this = (IdlStructTag&) tag;
	}

	void operator=(IdlStructTag& tag) {
	  name_ = tag.name_;
	  dims_ = tag.dims_;
	  type_ = tag.type_;
	  if(tag.def_)
	    def_  = new IdlStructDef(*tag.def_);
	  else
	    def_ = 0;
	};

	IdlStructTag() {
	  type_ = 0;
	  def_  = 0;
	};

	virtual ~IdlStructTag() {
	  if(def_)
	    delete def_;
	};
      };

      IdlStructDef();
      IdlStructDef(const IdlStructDef& def);
      IdlStructDef(IdlStructDef& def);
      
      // Assignment operator

      void operator=(const IdlStructDef& def);
      void operator=(IdlStructDef& def);

      virtual ~IdlStructDef();

      // Add a simple data member to a structure

      void addDataMember(std::string name,
			 gcp::util::MonitorDataType& dataType,
			 unsigned len=1);

      void addDataMember(std::string name,
			 gcp::util::MonitorDataType& dataType,
			 std::vector<unsigned int>& dims); 

      void addDataMember(std::string name,
			 gcp::util::DataType::Type dataType,
			 unsigned len=1);

      void addDataMember(std::string name,
			 gcp::util::DataType::Type dataType,
			 std::vector<unsigned int>& dims); 

      void addDataMember(std::string name,
			 int idlType,
			 std::vector<unsigned int>& dims); 

      // Add a substructure member to this structure

      IdlStructDef* addStructMember(std::string name,
				    IdlStructDef& def,
				    std::vector<unsigned int>& dims); 

      IdlStructDef* addStructMember(std::string name,
				    IdlStructDef& def,
				    unsigned len=1);

      IdlStructDef* addStructMember(std::string name);

      IdlStructDef* getStructMember(std::string name, bool create=false);
      
      // Convert a list of tags into a structure definition

      IDL_StructDefPtr getStructDefPtr();

      // Convert from our internal tag representation to the array
      // required by IDL

      IDL_STRUCT_TAG_DEF* createIdlTagArray();

      // Free a tag array previously allocated by createIdlTagArray()

      void deleteIdlTagArray(IDL_STRUCT_TAG_DEF* tags);

      // Create an IDL struct definition

      IDL_StructDefPtr makeStruct();

      void printTags();

      friend std::ostream& operator<<(std::ostream& os, IdlStructDef& def);

    public:

      std::list<IdlStructTag> tags_;

    private:

      std::vector<IdlStructDef*> structs_;

      IDL_StructDefPtr sDefPtr_;
    };

    // A class for managing IDL data structures

    class IdlHandler {
    public:

      // Constructor.

      IdlHandler(IDL_VPTR vptr);

      // Destructor.

      virtual ~IdlHandler();

      // Create a numeric array

      static IDL_VPTR createArray(std::vector<unsigned>& dims, 
				  gcp::util::DataType::Type dataType,
				  char** data);

      // Create a string

      static IDL_VPTR createString(std::string str);

      // Given a structure definition, create a structure

      static IDL_VPTR createStructure(IdlStructDef& def, std::vector<unsigned>& dims, char** data=0);
      static IDL_VPTR createStructure(IdlStructDef& def, unsigned len=1, char** data=0);

      // Utility function to return the IDL type code that corresponds
      // to the DataType code

      static int idlTypeOf(gcp::util::DataType::Type dataType);
      static int idlTypeOf(gcp::util::MonitorDataType& dataType);
      static int idlTypeOf(gcp::util::MonitorDataType::FormatType formatType);

    private:

      IDL_VPTR vptr_;

    }; // End class IdlHandler

  } // End namespace idl
} // End namespace gcp



#endif // End #ifndef GCP_IDL_IDLHANDLER_H
