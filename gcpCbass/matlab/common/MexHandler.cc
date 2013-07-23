#include "gcp/matlab/common/MexHandler.h"
#include "gcp/matlab/common/MexParser.h"

#include "gcp/util/common/Monitor.h"
#include "gcp/util/common/String.h"

#include <sstream>

using namespace std;

using namespace gcp::matlab;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
MexHandler::MexHandler() {}

/**.......................................................................
 * Destructor.
 */
MexHandler::~MexHandler() {}

/**.......................................................................
 * Methods for creating matlab arrays
 */
mxArray* MexHandler::createMatlabArray(int ndim, const int* dims, DataType::Type dataType)
{
  std::vector<mwSize> mtDims = convertDims(ndim, dims);

  switch (dataType) {
  case DataType::DATE:
  case DataType::STRING:
    return mxCreateCellArray((mwSize)ndim, (const mwSize*)&mtDims[0]);
  default:
    return mxCreateNumericArray((mwSize)ndim, (const mwSize*)&mtDims[0], matlabTypeOf(dataType), matlabComplexityOf(dataType));
    break;
  }
}

mxArray* MexHandler::createMatlabArray(int length, DataType::Type dataType)
{
  return createMatlabArray(1, &length, dataType);
}

mxArray* MexHandler::createMatlabArray(int ndim, const int* dims, MonitorDataType& dataType)
{
  return createMatlabArray(ndim, dims, dataType.selectedFormat);
}

mxArray* MexHandler::createMatlabArray(int ndim, const int* dims, 
				       MonitorDataType::FormatType formatType)
{
  std::vector<mwSize> mtDims = convertDims(ndim, dims);

  switch (formatType) {
  case MonitorDataType::FM_STRING:
  case MonitorDataType::FM_DATE:
    return mxCreateCellArray((mwSize)ndim, (const mwSize*)&mtDims[0]);
  default:
    return mxCreateNumericArray((mwSize)ndim, (const mwSize*)&mtDims[0], 
				matlabTypeOf(formatType), 
				matlabComplexityOf(formatType));
    break;
  }
}

double* MexHandler::createDoubleArray(mxArray** array, unsigned len)
{
  int dims[1];

  dims[0] = len;

  *array = createMatlabArray(1, dims, DataType::DOUBLE);
  return (double*)mxGetData(*array);
}

double* MexHandler::createDoubleArray(mxArray** array, int ndim, const int* dims)
{
  *array = createMatlabArray(ndim, dims, DataType::DOUBLE);
  return (double*)mxGetData(*array);
}

float* MexHandler::createFloatArray(mxArray** array, int ndim, const int* dims)
{
  *array = createMatlabArray(ndim, dims, DataType::FLOAT);
  return (float*)mxGetData(*array);
}

/**.......................................................................
 * Methods for returning the matlab complexity corresponding to a
 * dataType
 */
mxComplexity MexHandler::matlabComplexityOf(DataType::Type dataType)
{
  switch (dataType) {
  case DataType::COMPLEX_FLOAT:
    return mxCOMPLEX;
    break;
  default:
    return mxREAL;
    break;
  }
}

mxComplexity MexHandler::matlabComplexityOf(MonitorDataType& dataType)
{
  return matlabComplexityOf(dataType.selectedFormat);
}

mxComplexity MexHandler::matlabComplexityOf(MonitorDataType::FormatType formatType)
{
  switch (formatType) {
  case MonitorDataType::FM_COMPLEX_FLOAT:
    return mxCOMPLEX;
    break;
  default:
    return mxREAL;
    break;
  }
}

mxClassID MexHandler::matlabTypeOf(DataType::Type dataType)
{
  switch (dataType) {
  case DataType::BOOL:
  case DataType::UCHAR:
    return mxUINT8_CLASS;
    break;
  case DataType::CHAR:
    return mxINT8_CLASS;
    break;

    // Creating mxUINT16_CLASS arrays crashes matlab!  So I've
    // deliberately redefined this to create 32-bit ints.

  case DataType::USHORT:
    return mxUINT32_CLASS;
    break;
  case DataType::SHORT:
    return mxINT32_CLASS;
    break;
  case DataType::UINT:
    return mxUINT32_CLASS;
    break;
  case DataType::INT:
    return mxINT32_CLASS;
    break;
  case DataType::ULONG:
    return mxUINT64_CLASS;
    break;
  case DataType::LONG:
    return mxINT64_CLASS;
    break;
  case DataType::FLOAT:
    return mxSINGLE_CLASS;
    break;
  case DataType::COMPLEX_FLOAT:
    return mxSINGLE_CLASS;
    break;
  case DataType::DOUBLE:
    return mxDOUBLE_CLASS;
    break;
  }
}

mxClassID MexHandler::matlabTypeOf(MonitorDataType& dataType)
{
  return matlabTypeOf(dataType.selectedFormat);
}

mxClassID MexHandler::matlabTypeOf(MonitorDataType::FormatType formatType)
{
  switch (formatType) {
  case MonitorDataType::FM_BOOL:
  case MonitorDataType::FM_UCHAR:
    return mxUINT8_CLASS;
    break;
  case MonitorDataType::FM_CHAR:
    return mxINT8_CLASS;
    break;
  case MonitorDataType::FM_USHORT:
    return mxUINT16_CLASS;
    break;
  case MonitorDataType::FM_SHORT:
    return mxINT16_CLASS;
    break;
  case MonitorDataType::FM_UINT:
    return mxUINT32_CLASS;
    break;
  case MonitorDataType::FM_INT:
    return mxINT32_CLASS;
    break;
  case MonitorDataType::FM_ULONG:
    return mxUINT64_CLASS;
    break;
  case MonitorDataType::FM_LONG:
    return mxINT64_CLASS;
    break;
  case MonitorDataType::FM_FLOAT:
    return mxSINGLE_CLASS;
    break;
  case MonitorDataType::FM_COMPLEX_FLOAT:
    return mxSINGLE_CLASS;
    break;
  case MonitorDataType::FM_DOUBLE:
    return mxDOUBLE_CLASS;
    break;
  }
}

/**.......................................................................
 * Add a register block to the struct
 */
mxArray* MexHandler::addRegisterField(mxArray* ptr, RegDescription& reg)
{
  mxArray* regMapPtr = addNamedStructField(ptr,       reg.regMapName());
  mxArray* boardPtr  = addNamedStructField(regMapPtr, reg.boardName());
  mxArray* regPtr    = addNamedStructField(boardPtr,  reg.blockName());

  if(reg.aspect() != REG_PLAIN) {
    (void)addNamedStructField(regPtr,  reg.aspectName());
    return regPtr;
  } else {
    return boardPtr;
  }

}

/**.......................................................................
 * Add a register block to the struct, with native data types and
 * array of length nElements
 */
mxArray* MexHandler::addRegisterField(mxArray* basePtr, std::string regName, unsigned nElements)
{
  RegParser parser;
  RegDescription desc = parser.inputReg(regName);

  return addRegisterField(basePtr, desc, nElements);
}

/**.......................................................................
 * Add a register block to the struct, with native data types and
 * array of length nElements
 */
mxArray* MexHandler::addRegisterField(mxArray* basePtr, RegDescription& reg, unsigned nElements)
{
  mxArray* regMapPtr = addNamedStructField(basePtr,   reg.regMapName());
  mxArray* boardPtr  = addNamedStructField(regMapPtr, reg.boardName());
  mxArray* regPtr    = addNamedStructField(boardPtr,  reg.blockName());

  mxArray* ptr = 0;
  if(reg.aspect() != REG_PLAIN) {
    (void)addNamedStructField(regPtr,  reg.aspectName());
    ptr = regPtr;
  } else {
    ptr = boardPtr;
  }

  return createRegValArray(ptr, reg, Monitor::formatOf(reg), nElements);
}

/**.......................................................................
 * Create empty matlab arrays to be filled in by the monitor stream
 */
mxArray* MexHandler::createRegValArray(mxArray* ptr, 
				       RegDescription& reg,
				       MonitorDataType::FormatType formatType,
				       unsigned nFrame)
{
  int dims[4];
  unsigned nDim;
  CoordAxes axes = reg.axes();

  // Create and return the array in which values will be stored. We
  // want to format the array to have the same number of dimensions as
  // the C array.

  dims[0] = nFrame;
  if(formatType != MonitorDataType::FM_STRING) {
    nDim = axes.nAxis();
    for(unsigned iDim=0; iDim < nDim; iDim++)
      dims[iDim+1] = axes.nEl(iDim);

  } else {
    dims[1] = 1;
    nDim = 1;
  }

  mxArray* array = MexHandler::createMatlabArray(nDim+1, dims, formatType);
  
  mxSetField(ptr, 0, reg.aspect()==REG_PLAIN ? reg.blockName().c_str() :
  	     reg.aspectName().c_str(), array);
  
  return array;
}

/**.......................................................................
 * Add a named struct field to the passed structure.  
 */
mxArray* MexHandler::addHierNamedStructField(mxArray* parentPtr, 
					     std::string fieldName, 
					     gcp::util::DataType::Type dataType,
					     unsigned nDim, 
					     int* inDims)
{
  String name(fieldName);
  String last, next;

  mxArray* currPtr = parentPtr;
  mxArray* lastPtr = parentPtr;

  int dims[2] = {1,1};

  std::vector<mwSize> mtDims = convertDims(2, dims);

  do {

    next = name.findNextStringSeparatedByChars(".");

    if(!next.isEmpty()) {
  
      // Set the last pointer pointing to the current pointer so that
      // the next level of the structure is added hierarchically to
      // this one
      
      lastPtr = currPtr;
      last    = next;

      // Add this field if it doesn't already exist

      if((currPtr=mxGetField(lastPtr, 0, next.str().c_str()))==NULL) {
    
	mxAddField(lastPtr, next.str().c_str());
    
	// create empty register substructure
    
	currPtr = mxCreateStructArray((mwSize)2,(const mwSize*)&mtDims[0], 0, NULL);
    
	// link it to the parent structure
    
	mxSetField(lastPtr, 0, next.str().c_str(), currPtr);
      }
    }

  } while(!next.isEmpty());

  // If an array was to be allocated to this struct, add it now

  if(dataType != gcp::util::DataType::NONE) {

    std::vector<int> newDims;
    int* dimPtr=inDims;

    if(nDim < 2) {
      nDim = 2;
      newDims.resize(2);
      newDims[0] = 1;

      if(nDim == 0) {
	newDims[1] = 1;
      } else {
	newDims[1] = dimPtr[0];
      }

      dimPtr = &newDims[0];
    }

    mxArray* tmpPtr = createMatlabArray(nDim, dimPtr, dataType);
    mxSetField(lastPtr, 0, last.str().c_str(), tmpPtr);

    lastPtr = tmpPtr;
  }

  // Return a pointer to the last level added

  return lastPtr;
}

/**.......................................................................
 * Add a named struct field to the passed structure
 */
mxArray* MexHandler::addNamedStructField(mxArray* parentPtr, std::string fieldName, unsigned nElement, unsigned index)
{
  int dims[2]={nElement,1};
  
  std::vector<mwSize> mtDims = convertDims(2, dims);

  // Now add a field for the board
  
  mxArray* childPtr=NULL;
  
  if((childPtr=mxGetField(parentPtr, index, fieldName.c_str()))==NULL) {
    
    // Add it
    
    mxAddField(parentPtr, fieldName.c_str());
    
    // create empty register substructure
    
    childPtr = mxCreateStructArray((mwSize)2, (const mwSize*)&mtDims[0], 0, NULL);
    
    // link it to board structure
    
    mxSetField(parentPtr, index, fieldName.c_str(), childPtr);
  }
  
  return childPtr;
}

/**.......................................................................
 * Add a named string field to the passed structure
 */
char* MexHandler::addNamedStringStructField(mxArray* parentPtr, std::string fieldName, unsigned len, unsigned index)
{
  std::string str;
  str.resize(len);

  return addNamedStringStructField(parentPtr, fieldName, str, index);
}

/**.......................................................................
 * Add a named string field to the passed structure
 */
char* MexHandler::addNamedStringStructField(mxArray* parentPtr, std::string fieldName, std::string str, unsigned index)
{
  // Now add a field for the board
  
  mxArray* childPtr=NULL;
  
  if((childPtr=mxGetField(parentPtr, index, fieldName.c_str()))==NULL) {
    
    // Add it
    
    mxAddField(parentPtr, fieldName.c_str());
    
    // create empty register substructure
    
    childPtr = mxCreateString(str.c_str());
    
    // link it to board structure
    
    mxSetField(parentPtr, index, fieldName.c_str(), childPtr);
  }
  
  return (char*)mxGetData(childPtr);
}

/**.......................................................................
 * Add a named field to the passed structure, and create an unsigned
 * array for it
 */
unsigned* 
MexHandler::addNamedUintStructField(mxArray* parentPtr, std::string fieldName,
				      unsigned nElement, unsigned index)
{
  int dims[2] = {nElement, 1};

  addNamedStructField(parentPtr, fieldName);

  mxArray* array = MexHandler::createMatlabArray(2, dims, DataType::UINT);

  mxSetField(parentPtr, index, fieldName.c_str(), array);

  return (unsigned int*)mxGetData(array);
}

/**.......................................................................
 * Add a named field to the passed structure, and create a double
 * array for it
 */
double* 
MexHandler::addNamedDoubleStructField(mxArray* parentPtr, std::string fieldName,
				      unsigned nElement, unsigned index)
{
  int dims[2] = {nElement, 1};

  addNamedStructField(parentPtr, fieldName);

  mxArray* array = MexHandler::createMatlabArray(2, dims, DataType::DOUBLE);

  mxSetField(parentPtr, index, fieldName.c_str(), array);

  return (double*)mxGetData(array);
}

/**.......................................................................
 * Add a named field to the passed structure, and create a double
 * array for it
 */
double* 
MexHandler::addNamedDoubleStructField(mxArray* parentPtr, std::string fieldName,
				      std::vector<int> dims, unsigned index)
{
  addNamedStructField(parentPtr, fieldName);
  mxArray* array = MexHandler::createMatlabArray(dims.size(), &dims[0], DataType::DOUBLE);
  mxSetField(parentPtr, index, fieldName.c_str(), array);
  return (double*)mxGetData(array);
}

/**.......................................................................
 * Add a named field to the passed structure, and create a float
 * array for it
 */
float* 
MexHandler::addNamedFloatStructField(mxArray* parentPtr, std::string fieldName,
				      unsigned nElement, unsigned index)
{
  int dims[2] = {nElement, 1};

  addNamedStructField(parentPtr, fieldName);

  mxArray* array = MexHandler::createMatlabArray(2, dims, DataType::FLOAT);

  mxSetField(parentPtr, index, fieldName.c_str(), array);

  return (float*)mxGetData(array);
}

/**.......................................................................
 * Add a named field to the passed structure, and create a float
 * array for it
 */
float* 
MexHandler::addNamedFloatStructField(mxArray* parentPtr, std::string fieldName,
				      std::vector<int> dims, unsigned index)
{
  addNamedStructField(parentPtr, fieldName);
  mxArray* array = MexHandler::createMatlabArray(dims.size(), &dims[0], DataType::FLOAT);
  mxSetField(parentPtr, index, fieldName.c_str(), array);
  return (float*)mxGetData(array);
}

/**.......................................................................
 * Add a named struct field to the passed structure
 */
mxArray* MexHandler::addNamedCellField(mxArray* parentPtr, std::string fieldName, unsigned iDim, unsigned index)
{
  int dims[2]={1, iDim};
  
  std::vector<mwSize> mtDims = convertDims(2, dims);

  // Now add a field for the board
  
  mxArray* childPtr=NULL;
  
  if((childPtr=mxGetField(parentPtr, index, fieldName.c_str()))==NULL) {
    
    // Add it
    
    mxAddField(parentPtr, fieldName.c_str());
    
    // create empty register substructure
    
    childPtr = mxCreateCellArray((mwSize)2, (const mwSize*)&mtDims[0]);
    
    // link it to board structure
    
    mxSetField(parentPtr, index, fieldName.c_str(), childPtr);
  }
  
  return childPtr;
}

/**.......................................................................
 * Check arguments and complain if they do not match expectations
 */
void MexHandler::checkArgs(int nlhsExpected, int nrhsExpected, 
			   int nlhsActual,   int nrhsActual)
{
  std::ostringstream os;

  // Get the command-line arguments from matlab 
  
  if(nrhsActual != nrhsExpected) {
    os.str("");
    os << "Must have " << nrhsExpected << " input arguments\n";
    mexErrMsgTxt(os.str().c_str());
  }

  if(nlhsActual != nlhsExpected) {
    os.str("");
    os << "Must have " << nlhsExpected << " output arguments\n";
    mexErrMsgTxt(os.str().c_str());
  } 
}

LOG_HANDLER_FN(MexHandler::stdoutPrintFn)
{
  mexPrintf(logStr.c_str());
}

LOG_HANDLER_FN(MexHandler::stderrPrintFn)
{
  mexPrintf(logStr.c_str());
}

ERR_HANDLER_FN(MexHandler::throwFn)
{
  // For some reason, putting this line before the call to
  // mexErrMsgTxt() prevents a seg fault when mexErrMsgTxt() is called!

  std::cout << "" << std::endl;
  mexErrMsgTxt(os.str().c_str());
}

ERR_HANDLER_FN(MexHandler::reportFn)
{
  std::cout << "" << std::endl;
  mexPrintf(os.str().c_str());
}

ERR_HANDLER_FN(MexHandler::logFn)
{
  std::cout << "" << std::endl;
  mexPrintf(os.str().c_str());
}


mxArray* MexHandler::addNamedStructField(mxArray* parentPtr, std::string fieldName, int ndim, const int* dims, 
					 gcp::util::DataType::Type dataType, unsigned index)
{
  addNamedStructField(parentPtr, fieldName);

  mxArray* array = MexHandler::createMatlabArray(ndim, dims, dataType);

  mxSetField(parentPtr, index, fieldName.c_str(), array);

  return array;
}

/**.......................................................................
 * Add a named struct field that has the same dimensions and
 * type of the field named in copyName
 */
double* MexHandler::copyNamedDoubleStructField(mxArray* parentPtr, std::string fieldName, std::string copyName)
{
  const mxArray* fldPtr = MexParser::getField(parentPtr, copyName);
  const int ndim = MexParser::getNumberOfDimensions(fldPtr);
  const int* dims = MexParser::getDimensions(fldPtr);

  return (double*)mxGetData(addNamedStructField(parentPtr, fieldName, ndim, dims, DataType::DOUBLE, 0));
}

/**.......................................................................
 * Utility functions for calculating "multi-dimensional" array
 * indices into a one-dimensional representation.
 */

/**.......................................................................
 * Assumes that first index is the slowest-changing index (matlab-style)
 */
unsigned MexHandler::indexStartingWithSlowest(std::vector<unsigned>& coord, 
					      std::vector<unsigned>& dims)
{
  unsigned index=coord[coord.size()-1];
  for(int i=coord.size()-2; i >=0; i--)
    index = dims[i]*index + coord[i];

  return index;
}

/**.......................................................................
 * Assumes that first index is the fastest-changing index (C style)
 */
unsigned MexHandler::indexStartingWithFastest(std::vector<unsigned>& coord, 
					      std::vector<unsigned>& dims)
{
  unsigned index=coord[0];
  for(int i=1; i < coord.size(); i++)
    index = dims[i]*index + coord[i];

  return index;
}

/**.......................................................................
 * Return an array of C-order idices for the input dimensions.
 * Assumes dims is in order from fastest to slowest changing indices
 */
void MexHandler::getIndicesC(std::vector<unsigned>& cVec, 
			     std::vector<unsigned>& cDims)
{
  std::vector<unsigned> mVec;
  getIndices(cVec, mVec, cDims);
}

/**.......................................................................
 * Return an array of C-order idices for the input dimensions
 * Assumes dims is in order from slowest to fastest changing indices
 */
void MexHandler::getIndicesMatlab(std::vector<unsigned>& mVec, 
				  std::vector<unsigned>& mDims)
{
  std::vector<unsigned> cVec;

  // Invert the order of the matlab dimension array for input to the
  // getIndices() routine, which assumes C-style dimension ordering

  std::vector<unsigned> cDims(mDims.size());
  int iC=0, iM=mDims.size()-1;
  for(; iM >= 0; iM--, iC++) {
    cDims[iC] = mDims[iM];
  }
  
  getIndices(cVec, mVec, cDims);
}

/**.......................................................................
 * Return an array of C-order idices for the input dimensions
 * Assumes dims is in order from slowest to fastest changing indices
 */
void MexHandler::getIndicesMatlab(std::vector<unsigned>& mVec, 
				  std::vector<int>& mDims)
{
  std::vector<unsigned> cVec;

  // Invert the order of the matlab dimension array for input to the
  // getIndices() routine, which assumes C-style dimension ordering

  std::vector<unsigned> cDims(mDims.size());
  int iC=0, iM=mDims.size()-1;
  for(; iM >= 0; iM--, iC++) {
    cDims[iC] = mDims[iM];
  }
  
  getIndices(cVec, mVec, cDims);
}

/**.......................................................................
 * Return an array of C-order and Matlab-order idices for the input
 * dimensions
 */
void MexHandler::getIndices(std::vector<unsigned>& cVec, std::vector<unsigned>& mVec,
			    std::vector<unsigned>& dims, 
			    int iDim, unsigned indLast, unsigned nLast, unsigned baseLast)
{
  unsigned base=0;
  static unsigned iVecInd=0;
  static std::vector<unsigned> coord;

  if(iDim == 0) {
    unsigned n=1;
    for(unsigned i=0; i < dims.size(); i++)
      n *= dims[i];
    cVec.resize(n);
    mVec.resize(n);
    coord.resize(dims.size());
    iVecInd = 0;
  }

  if(iDim <= dims.size()) {
    base = baseLast*nLast + indLast;

    if(iDim > 0) 
      coord[iDim-1] = indLast;

  }

  if(iDim == dims.size()) {
    cVec[iVecInd] = base;
    mVec[iVecInd] = indexStartingWithSlowest(coord, dims);
    iVecInd++;
  }

  if(iDim < dims.size()) {
    for(unsigned indCurr=0; indCurr < dims[iDim]; indCurr++) {
      getIndices(cVec, mVec, dims, iDim+1, indCurr, iDim==0 ? 0 : dims[iDim], base);
    }
  }

  return;
}

unsigned MexHandler::getMatlabIndex(unsigned n1, unsigned n2, 
				    unsigned i1, unsigned i2)
{
  return i2 * n1 + i1;
}

unsigned MexHandler::getMatlabIndex(unsigned n1, unsigned n2, unsigned n3, 
				    unsigned i1, unsigned i2, unsigned i3)
{
  return i3 * (n1 * n2) + i2 * (n1) + i1;
}

double* MexHandler::createDoubleArray(mxArray** mxArray, MexParser& parser)
{
  return createDoubleArray(mxArray, 
			   parser.getNumberOfDimensions(), 
			   parser.getDimensions());
}

std::vector<mwSize> MexHandler::convertDims(int ndim, const int* dims)
{
  std::vector<mwSize> mtDims;
  mtDims.resize(ndim);

  for(unsigned i=0; i < ndim; i++) {
    mtDims[i] = dims[i];
  }

  return mtDims;
}

