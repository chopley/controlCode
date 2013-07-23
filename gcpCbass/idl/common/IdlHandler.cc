#include "gcp/idl/common/IdlHandler.h"

#include<iostream>

using namespace std;
using namespace gcp::idl;
using namespace gcp::util;

#define CHECK_DIMS(dims) \
  if(dims.size() > IDL_MAX_ARRAY_DIM) {\
    ThrowError("Requested number of dimensions (" << dims.size() \
             << ") exceeds the maximum allowed: " << IDL_MAX_ARRAY_DIM);\
  }

/**.......................................................................
 * Constructor.
 */
IdlHandler::IdlHandler(IDL_VPTR vptr) : vptr_(vptr) 
{}

/**.......................................................................
 * Destructor.
 */
IdlHandler::~IdlHandler() {}

/**.......................................................................
 * Create an unassigned array of the requested type
 */
IDL_VPTR IdlHandler::createArray(std::vector<unsigned>& dims, gcp::util::DataType::Type dataType,
				 char** data)
{
  CHECK_DIMS(dims);

  IDL_VPTR v=0;

  std::vector<IDL_MEMINT> idlDims;
  idlDims.resize(dims.size());

  for(unsigned iDim=0; iDim < dims.size(); iDim++)
    idlDims[iDim] = dims[iDim];

  *data = IDL_MakeTempArray(idlTypeOf(dataType), dims.size(), &idlDims[0], IDL_ARR_INI_ZERO, &v);
  
  return v;
}

/**.......................................................................
 * Create a string
 */
IDL_VPTR IdlHandler::createString(std::string str)
{
  return IDL_StrToSTRING((char*)str.c_str());
}

/**.......................................................................
 * Given a structure definition, create a structure
 */
IDL_VPTR IdlHandler::createStructure(IdlStructDef& def, unsigned len, char** data)
{
  std::vector<unsigned> dims;
  dims.push_back(len);
  return createStructure(def, dims, data);
}

/**.......................................................................
 * Given a structure definition, create a structure
 */
IDL_VPTR IdlHandler::createStructure(IdlStructDef& def, std::vector<unsigned>& dims, char** data)
{
  CHECK_DIMS(dims);

  IDL_VPTR v=0;

  // Given a structure definition, create a structure

  IDL_StructDefPtr s = def.getStructDefPtr();

  std::vector<IDL_MEMINT> idlDims;
  idlDims.resize(dims.size());

  for(unsigned iDim=0; iDim < dims.size(); iDim++)
    idlDims[iDim] = dims[iDim];

  // Now assign temporary memory to it and return a pointer to the data

  if(data)
    *data = IDL_MakeTempStruct(s, dims.size(), &idlDims[0], &v, TRUE);
  else
    (void)  IDL_MakeTempStruct(s, dims.size(), &idlDims[0], &v, TRUE);

  return v;
}

/**.......................................................................
 * Return the IDL type code that corresponds to the passsed DataType
 */
int IdlHandler::idlTypeOf(gcp::util::DataType::Type dataType)
{
  switch (dataType) {
  case DataType::BOOL:
  case DataType::UCHAR:
  case DataType::CHAR:
    return IDL_TYP_BYTE;
    break;
  case DataType::USHORT:
    return IDL_TYP_UINT;
    break;
  case DataType::SHORT:
    return IDL_TYP_INT;
    break;
  case DataType::UINT:
    return IDL_TYP_ULONG;
    break;
  case DataType::INT:
    return IDL_TYP_LONG;
    break;
  case DataType::ULONG:
    return IDL_TYP_ULONG64;
    break;
  case DataType::LONG:
    return IDL_TYP_LONG64;
    break;
  case DataType::FLOAT:
    return IDL_TYP_FLOAT;
    break;
  case DataType::DOUBLE:
    return IDL_TYP_DOUBLE;
    break;
  case DataType::STRING:
    return IDL_TYP_STRING;
    break;
  default:
    ThrowError("Unrecognized type");
    break;
  }
}

int IdlHandler::idlTypeOf(MonitorDataType& dataType)
{
  return idlTypeOf(dataType.selectedFormat);
}

int IdlHandler::idlTypeOf(MonitorDataType::FormatType formatType)
{
  switch (formatType) {
  case MonitorDataType::FM_BOOL:
  case MonitorDataType::FM_UCHAR:
  case MonitorDataType::FM_CHAR:
    return IDL_TYP_BYTE;
    break;
  case MonitorDataType::FM_USHORT:
    return IDL_TYP_UINT;
    break;
  case MonitorDataType::FM_SHORT:
    return IDL_TYP_INT;
    break;
  case MonitorDataType::FM_UINT:
    return IDL_TYP_ULONG;
    break;
  case MonitorDataType::FM_INT:
    return IDL_TYP_LONG;
    break;
  case MonitorDataType::FM_ULONG:
    return IDL_TYP_ULONG64;
    break;
  case MonitorDataType::FM_LONG:
    return IDL_TYP_LONG64;
    break;
  case MonitorDataType::FM_FLOAT:
    return IDL_TYP_FLOAT;
    break;
  case MonitorDataType::FM_DOUBLE:
    return IDL_TYP_DOUBLE;
    break;
  case MonitorDataType::FM_DATE:
  case MonitorDataType::FM_STRING:
    return IDL_TYP_STRING;
    break;
  }
}

//=======================================================================
// Methods of IdlStructDef
//=======================================================================

IdlStructDef::IdlStructDef()
{
  sDefPtr_ = 0;
  tags_.clear();
}

/**.......................................................................
 * Const copy constructor
 */
IdlStructDef::IdlStructDef(const IdlStructDef& def)
{
  *this = (IdlStructDef&)def;
}


/**.......................................................................
 * Copy constructor
 */
IdlStructDef::IdlStructDef(IdlStructDef& def)
{
  *this = def;
}

/**.......................................................................
 * Assignment operator
 */
void IdlStructDef::operator=(const IdlStructDef& def)
{
  *this = (IdlStructDef&)def;
}

/**.......................................................................
 * Assignment operator
 */
void IdlStructDef::operator=(IdlStructDef& def)
{
  // Resize the tags array

  tags_ = def.tags_;

  // Copy the structure def ptr if relevant

  sDefPtr_ = def.sDefPtr_;
}

IdlStructDef::~IdlStructDef() {}

/**.......................................................................
 * Add a simple data member to a structure
 */
void IdlStructDef::addDataMember(std::string name,
				 gcp::util::MonitorDataType& dataType,
				 unsigned len)
{
  std::vector<unsigned> dims;
  dims.push_back(len);
  addDataMember(name, dataType, dims);
}

void IdlStructDef::addDataMember(std::string name,
				 gcp::util::MonitorDataType& dataType,
				 std::vector<unsigned int>& dims) 
{
  addDataMember(name, IdlHandler::idlTypeOf(dataType), dims);
}

/**.......................................................................
 * Add a simple data member to a structure
 */
void IdlStructDef::addDataMember(std::string name,
				 gcp::util::DataType::Type dataType,
				 unsigned len)
{
  std::vector<unsigned> dims;
  dims.push_back(len);
  addDataMember(name, dataType, dims);
}

void IdlStructDef::addDataMember(std::string name,
				 gcp::util::DataType::Type dataType,
				 std::vector<unsigned int>& dims) 
{
  addDataMember(name, IdlHandler::idlTypeOf(dataType), dims);
}

void IdlStructDef::addDataMember(std::string name,
				 int idlType,
				 std::vector<unsigned int>& dims) 
{
  IdlStructTag tag;

  tag.name_ = name;
  tag.dims_ = dims;
  tag.type_ = (void*)idlType;
  tag.def_  = 0;

  // Insert this tag at the end of the list of known tags
 
  tags_.push_back(tag);
}

/**.......................................................................
 * Add a blank substructure member to this structure
 */
IdlStructDef* IdlStructDef::addStructMember(std::string name)
{
  IdlStructDef def;
  return addStructMember(name, def);
}


/**.......................................................................
 * Add a substructure member to this structure
 */
IdlStructDef* IdlStructDef::addStructMember(std::string name,
					    IdlStructDef& def,
					    unsigned len)
{
  std::vector<unsigned> dims;
  dims.push_back(len);
  return addStructMember(name, def, dims);
}

IdlStructDef* IdlStructDef::addStructMember(std::string name,
					    IdlStructDef& def,
					    std::vector<unsigned int>& dims) 
{
  IdlStructTag tag;

  tag.name_ = name;
  tag.dims_ = dims;
  tag.type_ = 0;
  tag.def_  = new IdlStructDef(def);

  // Insert this tag at the end of the list of known tags

  tags_.push_back(tag);

  // Return the IdlStructDef pointer we just added

  return tags_.back().def_;
}

/**.......................................................................
 * Return the descriptor for the matching structure member.  If
 * create=true, this call will create the structure member if it
 * doesn't exist already.
 */
IdlStructDef* IdlStructDef::getStructMember(std::string name, bool create) 
{
  // Look for a match amongst valid tags
  
  for(std::list<IdlStructTag>::iterator tag = tags_.begin(); tag != tags_.end(); ++tag) {

    // But only tags that represent structs

    if(tag->def_ != 0) {

      // If this tag matches, return the struct description that
      // corresponds to it

      if(strcmp((char*)name.c_str(), (char*)tag->name_.c_str())==0)
	return tag->def_;
    }
  }

  // If no match was found, create the structure member if requested,
  // else return null

  if(create)
    return addStructMember(name);
  else
    return 0;
}

/**.......................................................................
 * Convert a list of tags into a structure definition
 */
IDL_StructDefPtr IdlStructDef::getStructDefPtr()
{
  // If this def is already made, return the old one

  if(sDefPtr_ != 0)
    return sDefPtr_;

  // Iterate through our list of tags.  Any tag that is a sub
  // structure must have its type field filled in with the appropriate
  // structure def ptr.

  for(std::list<IdlStructTag>::iterator tag=tags_.begin(); tag != tags_.end(); tag++) {

    if(tag->def_ != 0) {
      tag->type_ = tag->def_->getStructDefPtr();
    }
  }

  // Now return our own structure definition

  return makeStruct();
}

/**.......................................................................
 * Create an IDL struct definition
 */
IDL_StructDefPtr IdlStructDef::makeStruct()
{
  // Convert from our tag array to the tag array that the IDL call
  // requires

  IDL_STRUCT_TAG_DEF* tags=0;
  IDL_StructDefPtr s = 0;

  try {
    tags = createIdlTagArray();
    s = IDL_MakeStruct(0, tags);
  } catch(...) {
  }

  deleteIdlTagArray(tags);

  return s;
}

/**.......................................................................
 * Convert from our internal tag representation to the array required
 * by IDL
 */
IDL_STRUCT_TAG_DEF* IdlStructDef::createIdlTagArray()
{
  if(tags_.size() == 0) {
    ThrowError("Received a structure with zero tags!");
  }

  IDL_STRUCT_TAG_DEF* tags = 0;

  // Allocate enough memory for the tags + 1 since the last element
  // must be null

  tags = (IDL_STRUCT_TAG_DEF*)malloc(sizeof(IDL_STRUCT_TAG_DEF) * (tags_.size()+1));

  if(tags == 0) {
    ThrowError("Unable to allocate memory for tag array");
  }

  // Initialize memory

  for(unsigned i=0; i < tags_.size()+1; i++) {
    tags[i].name  = 0;
    tags[i].dims  = 0;
    tags[i].type  = 0;
    tags[i].flags = 0;
  }

  std::list<IdlStructTag>::iterator tag=tags_.begin();
  for(unsigned i=0; i < tags_.size(); i++, tag++) {
    tags[i].name = (char*)malloc(strlen(tag->name_.c_str())+1);

    if(tags[i].name==0) {
      ThrowError("Unable to allocate memory for tag[" << i << "] name");
    }
    
    strcpy(tags[i].name, (char*)tag->name_.c_str());
    for(unsigned iChar=0; iChar < strlen(tags[i].name); iChar++)
      tags[i].name[iChar] = (char)toupper(tags[i].name[iChar]);

    tags[i].dims = (IDL_MEMINT*)malloc(sizeof(IDL_MEMINT) * (tag->dims_.size() + 1));

    if(tags[i].dims==0) {
      ThrowError("Unable to allocate memory for tag[" << i << "] dims");
    }
    
    tags[i].dims[0] = tag->dims_.size();
    for(unsigned iDim=0; iDim < tag->dims_.size(); iDim++)
      tags[i].dims[iDim+1] = tag->dims_[iDim];

    tags[i].type = tag->type_;
  }

  return tags;
}

/**.......................................................................
 * Free a tag array previously allocated by createIdlTagArray()
 */
void IdlStructDef::deleteIdlTagArray(IDL_STRUCT_TAG_DEF* tags)
{
  if(tags) {

    for(unsigned i=0; i < tags_.size()+1; i++) {

      if(tags[i].name) {
	free(tags[i].name);
      }

      if(tags[i].dims)
	free(tags[i].dims);
    }

    free(tags);
  }
}

void IdlStructDef::printTags()
{
  std::list<IdlStructTag>::iterator tag=tags_.begin();
  for(unsigned i=0; i < tags_.size(); tag++, i++) {
    COUT("Tag " << i << " = " << tag->name_ << " " << tag->dims_.size() << " " << tag->type_);
  }
}

ostream& 
gcp::idl::operator<<(std::ostream& os, IdlStructDef& def)
{
  std::list<IdlStructDef::IdlStructTag>::iterator tag=def.tags_.begin();
  for(unsigned i=0; i < def.tags_.size(); tag++, i++) {
    os << "Tag " << i << " = " << tag->name_ << " " << tag->dims_.size() 
       << " " << tag->type_ << std::endl;
    if(tag->def_ != 0)
      os << "Sub Struct: " << *tag->def_ << std::endl;
  }

  return os;
}
