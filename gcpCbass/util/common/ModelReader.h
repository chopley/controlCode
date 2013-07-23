// $Id: ModelReader.h,v 1.1 2010/11/23 01:37:09 sjcm Exp $

#ifndef CBASS_MATLAB_MODELREADER_H
#define CBASS_MATLAB_MODELREADER_H

/**
 * @file ModelReader.h
 * 
 * Tagged: Thu Sep  8 18:35:46 PDT 2005
 * 
 * @version: $Revision: 1.1 $, $Date: 2010/11/23 01:37:09 $
 * 
 * @author Erik Leitch
 */
#include <string>
#include<vector>

#include "gcp/util/common/QuadraticInterpolatorNormal.h"
#include "gcp/util/common/TimeVal.h"

class InputStream;

namespace gcp {
  namespace util {

    class ModelReader {
    public:

      // Enumerate interpolation error codes

      enum {
	ERR_NONE         = 0x0,
	ERR_OUTSIDE_MJD  = 0x1,
	ERR_OUTSIDE_FREQ = 0x2,
      };

      static const double arcSecPerRad_;

      /**
       * Constructor.
       */
      ModelReader();
      ModelReader(std::string dir, std::string fileName);

      /**
       * Destructor.
       */
      virtual ~ModelReader();

      void readFile(std::string dir, std::string fileName);

      double getDistance(TimeVal& mjd, unsigned int& errCode);
      double getDistance(double mjd, unsigned int& errCode);

      void readRecord(InputStream* stream);
      void readItem(InputStream* stream);

    public:

      std::vector<double> mjd_;
      std::vector<double> ra_;
      std::vector<double> dec_;
      std::vector<double> dist_;

      QuadraticInterpolator* raInterp_;
      QuadraticInterpolator* decInterp_;
      QuadraticInterpolator* distInterp_;

    public:
      void fillInterpContainers(double mjd);

    }; // End class ModelReader

  } // End namespace matlab
} // End namespace cbass



#endif // End #ifndef CBASS_MATLAB_MODELREADER_H
