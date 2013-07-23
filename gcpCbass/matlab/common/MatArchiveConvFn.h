// $Id: MatArchiveConvFn.h,v 1.2 2010/03/03 18:23:12 eml Exp $

#ifndef GCP_MATLAB_MATARCHIVECONVFN_H
#define GCP_MATLAB_MATARCHIVECONVFN_H

/**
 * @file MatArchiveConvFn.h
 * 
 * Tagged: Mon Feb  2 01:14:35 NZDT 2009
 * 
 * @version: $Revision: 1.2 $, $Date: 2010/03/03 18:23:12 $
 * 
 * @author username: Command not found.
 */

#include "gcp/util/common/DataType.h"
#include "gcp/util/common/RegDescription.h"

#include "gcp/matlab/common/MexHandler.h"

#include <iostream>
#include <valarray>


#define MAT_ARC_CONV_FN(fn) void (fn)(gcp::util::RegDescription& reg, gcp::matlab::MexHandler::MxArray& array, unsigned char* raw, double* cal, unsigned& iFrame, unsigned& nFrame, unsigned& nEl, unsigned int* cInds, unsigned int* mInds)

namespace gcp {
  namespace matlab {

    class MatArchiveConvFn {
    public:

      static MAT_ARC_CONV_FN(toBool);
      static MAT_ARC_CONV_FN(toUchar);
      static MAT_ARC_CONV_FN(toChar);
      static MAT_ARC_CONV_FN(toUshort);
      static MAT_ARC_CONV_FN(toShort);
      static MAT_ARC_CONV_FN(toUint);
      static MAT_ARC_CONV_FN(toInt);
      static MAT_ARC_CONV_FN(toUlong);
      static MAT_ARC_CONV_FN(toLong);
      static MAT_ARC_CONV_FN(toFloat);
      static MAT_ARC_CONV_FN(toDouble);
      static MAT_ARC_CONV_FN(toDate);
      static MAT_ARC_CONV_FN(toString);

      static void setConvFn(gcp::util::DataType::Type outputType, RegAspect aspect, MAT_ARC_CONV_FN(**fptr));

      /**
       * Constructor.
       */
      MatArchiveConvFn();

      /**
       * Copy Constructor.
       */
      MatArchiveConvFn(const MatArchiveConvFn& objToBeCopied);

      /**
       * Copy Constructor.
       */
      MatArchiveConvFn(MatArchiveConvFn& objToBeCopied);

      /**
       * Const Assignment Operator.
       */
      void operator=(const MatArchiveConvFn& objToBeAssigned);

      /**
       * Assignment Operator.
       */
      void operator=(MatArchiveConvFn& objToBeAssigned);

      /**
       * Output Operator.
       */
      friend std::ostream& operator<<(std::ostream& os, MatArchiveConvFn& obj);

      /**
       * Destructor.
       */
      virtual ~MatArchiveConvFn();

    private:
    }; // End class MatArchiveConvFn

    std::ostream& operator<<(std::ostream& os, MatArchiveConvFn& obj);

  } // End namespace matlab
} // End namespace gcp



#endif // End #ifndef GCP_MATLAB_MATARCHIVECONVFN_H
