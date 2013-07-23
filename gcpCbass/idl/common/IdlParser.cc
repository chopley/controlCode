#include "gcp/idl/common/IdlParser.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

#include<iostream>

using namespace std;
using namespace gcp::idl;
using namespace gcp::util;

#define CHECK_VPTR(vPtr) \
  {\
    if(vPtr == 0)\
      ThrowError("Null array received");\
  }
    
/**.......................................................................
 * Constructor.
 */
IdlParser::IdlParser(const IDL_VPTR vPtr) 
{
  setTo(vPtr);
}

IdlParser::IdlParser()
{
  vPtr_     = 0;
  dataBase_ = 0;
}

void IdlParser::setTo(const IDL_VPTR vPtr) 
{
  CHECK_VPTR(vPtr);
  vPtr_ = vPtr;
  dataBase_ = getBaseDataPtr();
}

/**.......................................................................
 * Destructor.
 */
IdlParser::~IdlParser() {}

/**.......................................................................
 * Return the dimensionality of an mxVPtr
 */
int IdlParser::getNumberOfDimensions()
{
  CHECK_VPTR(vPtr_);

  if(vPtr_->flags & IDL_V_ARR) {
    return (int)vPtr_->value.arr->n_dim;
  } else {
    return 1;
  }
}

/**.......................................................................
 * Return the vector of dimensions for this vPtr
 */
std::vector<unsigned> IdlParser::getDimensions()
{
  CHECK_VPTR(vPtr_);

  std::vector<unsigned> dims;
  dims.resize(getNumberOfDimensions());

  if(vPtr_->flags & IDL_V_ARR) {
    for(unsigned i=0; i < dims.size(); i++)
      dims[i] = vPtr_->value.arr->dim[i];
  } else {
    dims[0] = 1;
  }

  return dims;
}

/**.......................................................................
 * Return the number of elements in a dimension
 */
unsigned IdlParser::getDimension(unsigned iDim)
{
  if(iDim > getNumberOfDimensions()-1)
    ThrowError("Dimension: " << iDim << " too large for this vPtr");

  std::vector<unsigned> dims = getDimensions();

  return dims[iDim];
}

/**.......................................................................
 * Print the dimensionality of this vPtr
 */
void IdlParser::printDimensions()
{
  int ndim;

  ndim = getNumberOfDimensions();
  std::vector<unsigned> dims = getDimensions();

  std::cout << "Dimensions are: ";
  for(unsigned idim=0; idim < ndim; idim++)
    std::cout << "[" << dims[idim] << "]";
  std::cout << std::endl;
}

/**.......................................................................
 * Get a pointer to double data
 */
double* IdlParser::getDoubleData()
{
  CHECK_VPTR(vPtr_);

  if(!isDouble())
    ThrowError("Variable does not represent double precision data");

  return (double*)getPtrToData();
}

/**.......................................................................
 * Return true if this variable is of type struct
 */
bool IdlParser::isStruct()
{
  CHECK_VPTR(vPtr_);
  return vPtr_->flags & IDL_V_STRUCT;
}

/**.......................................................................
 * Return true if this variable is an array
 */
bool IdlParser::isArray()
{
  CHECK_VPTR(vPtr_);
  return vPtr_->flags & IDL_V_ARR;
}

/**.......................................................................
 * Return true if this variable is of type double
 */
bool IdlParser::isDouble()
{
  CHECK_VPTR(vPtr_);
  return vPtr_->type == IDL_TYP_DOUBLE;
}

/**.......................................................................
 * Return true if this variable is of type string
 */
bool IdlParser::isString()
{
  CHECK_VPTR(vPtr_);
  return vPtr_->type == IDL_TYP_STRING;
}

/**.......................................................................
 * Get the number of tags in this structure
 */
unsigned IdlParser::getNumberOfTags()
{
  if(!isStruct())
    ThrowError("Not a structure");

  return (unsigned) IDL_StructNumTags(vPtr_->value.s.sdef);  
}

/**.......................................................................
 * Get info about a structure tag
 */
IdlParser IdlParser::getTag(std::string tagName)
{
  if(!isStruct())
    ThrowError("Not a structure");

  // Copy the tagName

  std::string tagCopy = tagName;
  for(unsigned i=0; i < tagCopy.size(); i++)
    tagCopy[i] = (char)toupper((int)tagCopy[i]);

  IDL_VPTR tmp=0;
  char* structName=0;

  IdlParser tag;
  IDL_StructTagInfoByName(vPtr_->value.s.sdef, (char*)tagCopy.c_str(), 0, &tag.vPtr_);
  tag.name_     = tagName;
  
  // The info returned above for the tag doesn't contain a valid
  // data pointer.  Thus we need to store the base pointer to the
  // real data here
  
  tag.dataBase_ = getPtrToDataForTag(tagCopy);

  return tag;
}

/**.......................................................................
 * Get a list of all tags in this structure
 */
std::vector<IdlParser> IdlParser::getTagList()
{
  if(!isStruct())
    ThrowError("Not a structure");

  std::vector<IdlParser> tags;

  tags.resize(getNumberOfTags());

  IDL_VPTR tmp=0;
  char* structName=0;

  for(unsigned iTag=0; iTag < tags.size(); iTag++) {
    IDL_StructTagInfoByIndex(vPtr_->value.s.sdef, iTag, 0, &tags[iTag].vPtr_);
    tags[iTag].name_     = IDL_StructTagNameByIndex(vPtr_->value.s.sdef, iTag, 0, &structName);

    // The info returned above for the tag doesn't contain a valid
    // data pointer.  Thus we need to store the base pointer to the
    // real data here

    tags[iTag].dataBase_ = getPtrToDataForTag(tags[iTag].name_);
  }

  return tags;
}

unsigned IdlParser::dataOffsetOfTag(std::string tagName)
{
  if(!isStruct())
    ThrowError("Not a structure");

  // Copy the tagName

  std::string tagCopy = tagName;
  for(unsigned i=0; i < tagCopy.size(); i++)
    tagCopy[i] = (char)toupper((int)tagCopy[i]);

  IDL_VPTR tmp=0;
  return IDL_StructTagInfoByName(vPtr_->value.s.sdef, (char*)tagCopy.c_str(), 0, &tmp);
}

char* IdlParser::getPtrToDataForTag(std::string tagName)
{
  return getPtrToData() + dataOffsetOfTag(tagName);
}

char* IdlParser::getPtrToData()
{
  return dataBase_;
}

char* IdlParser::getBaseDataPtr()
{
  char* cPtr=0;

  IDL_MEMINT nEl;
  IDL_VarGetData(vPtr_, &nEl, &cPtr, FALSE);

  return cPtr;
}

unsigned IdlParser::getNumberOfElements()
{
  IDL_MEMINT nEl;
  char* cPtr=0;
  IDL_VarGetData(vPtr_, &nEl, &cPtr, FALSE);

  return nEl;
}

unsigned IdlParser::getDataSizeOfElement()
{
  if(isArray()) {
    return vPtr_->value.arr->elt_len;
  } else {
    return getSizeOf(vPtr_->flags);
  }
}

unsigned IdlParser::getSizeOf(unsigned idlFlag)
{
  if(idlFlag & IDL_TYP_BYTE)
    return sizeof(vPtr_->value.c);
  else if(idlFlag & IDL_TYP_INT)
    return sizeof(vPtr_->value.i);
  else if(idlFlag & IDL_TYP_UINT)
    return sizeof(vPtr_->value.ui);
  else if(idlFlag & IDL_TYP_LONG)
    return sizeof(vPtr_->value.l);
  else if(idlFlag & IDL_TYP_ULONG)
    return sizeof(vPtr_->value.ul);
  else if(idlFlag & IDL_TYP_LONG64)
    return sizeof(vPtr_->value.l64);
  else if(idlFlag & IDL_TYP_ULONG64)
    return sizeof(vPtr_->value.ul64);
  else if(idlFlag & IDL_TYP_FLOAT)
    return sizeof(vPtr_->value.f);
  else if(idlFlag & IDL_TYP_DOUBLE)
    return sizeof(vPtr_->value.d);
  else
    return 0;
}

std::string IdlParser::getString(unsigned index)
{
  if(!isString())
    ThrowError("Array does not represent a string");

  if(index > getNumberOfElements()-1)
    ThrowError("Index: " << index 
	       << " exceeds the maximum for this variable (" 
	       << getNumberOfElements()-1 << ")"); 

  return IDL_STRING_STR((IDL_STRING*)getPtrToData() + index);
}
