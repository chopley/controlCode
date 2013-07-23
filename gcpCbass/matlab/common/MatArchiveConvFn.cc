#include "gcp/matlab/common/MatArchiveConvFn.h"

#include "gcp/util/common/CoordAxes.h"

#include<iostream>

using namespace std;

using namespace gcp::matlab;
using namespace gcp::util;

#if 0
#define ASSIGN_VALS(cast) \
  {\
    CoordAxes& axes = reg.axes_;\
    cast* ptr = (cast*)array.vPtr_;\
    unsigned nEl0 = axes.nEl(0);\
    unsigned nEl1 = axes.nAxis()==2 ? axes.nEl(1) : 1;\
    unsigned iM, iC;\
    for(unsigned iEl1=0; iEl1 < nEl1; iEl1++) {\
      for(unsigned iEl0=0; iEl0 < nEl0; iEl0++) {\
        iC = iEl0*nEl1 + iEl1;\
        iM = iEl1*nFrame*nEl0 + iEl0*nFrame + iFrame;\
        *(ptr + iM) = (cast)*(cal + iC);\
      }\
    }\
  }
#else
#define ASSIGN_VALS(cast) \
  {\
    unsigned nEl0, nEl1;\
    cast* ptr = (cast*)array.vPtr_;\
    for(unsigned iEl=0; iEl < nEl; iEl++) {\
      *(ptr + *(mInds+iEl) + iFrame) = (cast)*(cal + *(cInds+iEl));\
    }\
  }
#endif

MAT_ARC_CONV_FN(MatArchiveConvFn::toUchar)
{
  ASSIGN_VALS(unsigned char);
}

MAT_ARC_CONV_FN(MatArchiveConvFn::toChar)
{
  ASSIGN_VALS(char);
}

MAT_ARC_CONV_FN(MatArchiveConvFn::toUshort)
{
  ASSIGN_VALS(unsigned short);
}

MAT_ARC_CONV_FN(MatArchiveConvFn::toShort)
{
  ASSIGN_VALS(short);
}

MAT_ARC_CONV_FN(MatArchiveConvFn::toUint)
{
  ASSIGN_VALS(unsigned int);
}

MAT_ARC_CONV_FN(MatArchiveConvFn::toInt)
{
  ASSIGN_VALS(int);
}

MAT_ARC_CONV_FN(MatArchiveConvFn::toUlong)
{
  ASSIGN_VALS(unsigned long);
}

MAT_ARC_CONV_FN(MatArchiveConvFn::toLong)
{
  ASSIGN_VALS(long);
}

MAT_ARC_CONV_FN(MatArchiveConvFn::toFloat)
{
#if 0
#if 1
  float* ptr = (float*)array.vPtr_;
  for(unsigned iEl=0; iEl < nEl; iEl++) {
    *(ptr + *(mInds+iEl) + iFrame) = (float)*(cal + *(cInds+iEl));
  }
#else
  ASSIGN_VALS(float);
#endif
#endif
}

MAT_ARC_CONV_FN(MatArchiveConvFn::toDouble)
{
  ASSIGN_VALS(double);
}

MAT_ARC_CONV_FN(MatArchiveConvFn::toDate)
{
  CoordAxes& axes = reg.axes_;\

  static RegDate date;
  RegDate::Data* ptr = (RegDate::Data*)raw;

  unsigned nEl0 = axes.nEl(0);
  unsigned nEl1 = axes.nAxis()==2 ? axes.nEl(1) : 1;
  unsigned iM, iC;

  for(unsigned iEl1=0; iEl1 < nEl1; iEl1++) {
    for(unsigned iEl0=0; iEl0 < nEl0; iEl0++) {
      iC = iEl0*nEl1 + iEl1;
      iM = iEl1*nFrame*nEl0 + iEl0*nFrame + iFrame;

      date = *(ptr + iC);
      mxSetCell(array.array_, iM, mxCreateString(date.str().c_str()));
    }
  }
}

MAT_ARC_CONV_FN(MatArchiveConvFn::toString)
{
  static std::ostringstream os;
  unsigned len = reg.nEl();

  os.str("");
  for(unsigned i=0; i < len; i++)
    os << *(raw+i);
  
  mxSetCell(array.array_, iFrame, mxCreateString(os.str().c_str()));
}

void MatArchiveConvFn::setConvFn(DataType::Type outputType, RegAspect aspect, MAT_ARC_CONV_FN(**fptr))
{
  switch (outputType) {
  case DataType::BOOL:
    *fptr = &toUchar;
    break;
  case DataType::UCHAR:
    *fptr = &toUchar;
    break;
  case DataType::CHAR:
    *fptr = &toChar;
    break;
  case DataType::USHORT:
    *fptr = &toUshort;
    break;
  case DataType::SHORT:
    *fptr = &toShort;
    break;
  case DataType::UINT:
    *fptr = &toUint;
    break;
  case DataType::INT:
    *fptr = &toInt;
    break;
  case DataType::ULONG:
    *fptr = &toUlong;
    break;
  case DataType::LONG:
    *fptr = &toLong;
    break;
  case DataType::FLOAT:
    *fptr = &toFloat;
    break;
  case DataType::DOUBLE:
    *fptr = &toDouble;
    break;
  case DataType::DATE:
    *fptr = &toDate;
    break;
  case DataType::STRING:
    *fptr = &toString;
    break;
  default:
    break;
  }
}

/**.......................................................................
 * Constructor.
 */
MatArchiveConvFn::MatArchiveConvFn() {}

/**.......................................................................
 * Const Copy Constructor.
 */
MatArchiveConvFn::MatArchiveConvFn(const MatArchiveConvFn& objToBeCopied)
{
  *this = (MatArchiveConvFn&)objToBeCopied;
};

/**.......................................................................
 * Copy Constructor.
 */
MatArchiveConvFn::MatArchiveConvFn(MatArchiveConvFn& objToBeCopied)
{
  *this = objToBeCopied;
};

/**.......................................................................
 * Const Assignment Operator.
 */
void MatArchiveConvFn::operator=(const MatArchiveConvFn& objToBeAssigned)
{
  *this = (MatArchiveConvFn&)objToBeAssigned;
};

/**.......................................................................
 * Assignment Operator.
 */
void MatArchiveConvFn::operator=(MatArchiveConvFn& objToBeAssigned)
{
  std::cout << "Calling default assignment operator for class: MatArchiveConvFn" << std::endl;
};

/**.......................................................................
 * Output Operator.
 */
std::ostream& gcp::matlab::operator<<(std::ostream& os, MatArchiveConvFn& obj)
{
  os << "Default output operator for class: MatArchiveConvFn" << std::endl;
  return os;
};

/**.......................................................................
 * Destructor.
 */
MatArchiveConvFn::~MatArchiveConvFn() {}
