// $Id: IdlGenerator.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_IDLGENERATOR_H
#define GCP_UTIL_IDLGENERATOR_H

/**
 * @file IdlGenerator.h
 * 
 * Tagged: Tue Jul 24 11:13:44 PDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author username: Command not found.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace gcp {
  namespace util {

    class IdlGenerator {
    public:

      /**
       * Constructor.
       */
      IdlGenerator(std::string fileName);
      IdlGenerator(std::string fileName, std::string dir);

      void setOutputPrefix(std::string prefix);
      void setOutputCcSuffix(std::string suffix);

      /**
       * Destructor.
       */
      virtual ~IdlGenerator();

      void outputHeaderFile();
      void outputLoadFile();
      void outputDlmFile();
      void outputCcFile();


    private:

      std::string outputPrefix_;
      std::string outputCcSuffix_;

      void parseFile();
      std::string caps(std::string inp);

      std::string sourceFileName_;
      std::string sourceFilePrefix_;
      std::string dir_;

      std::vector<std::string> functions_;
      std::vector<std::string> procedures_;
      std::vector<std::string> procRetVals_;

    }; // End class IdlGenerator

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_IDLGENERATOR_H
