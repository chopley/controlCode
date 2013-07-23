#ifndef GCP_UTIL_HTMLDOC_H
#define GCP_UTIL_HTMLDOC_H

/**
 * @file HtmlDoc.h
 * 
 * Tagged: Wed Apr 11 22:59:31 PDT 2007
 * 
 * @author Erik Leitch
 */
#include "gcp/control/code/unix/libscript_src/script.h"

namespace gcp {
  namespace util {
    
    class HtmlDoc {
    public:
      
      /**
       * Constructor.
       */
      HtmlDoc();
      
      /**
       * Destructor.
       */
      virtual ~HtmlDoc();

      static void generateAutoDocumentation(Script* sc, std::string dir);
      static void createDirs(std::string& dir);
      static void writeHtmlStyleSheet(Script* sc, std::string& dir);
      static void writeHtmlCommandIndexFile(Script* sc, std::string& dir);
      static void writeHtmlCommandList(Script* sc, std::string& dir);
      static void writeHtmlHeader(std::ofstream& fout, std::string path);
      static void writeHtmlFooter(std::ofstream& fout);
      static void writeHtmlCommandSynopsisFile(std::string& dir, ScriptCmd& cmd);
      static void writeHtmlCommandUsageFile(std::string& dir, ScriptCmd& cmd);
      static void writeHtmlFunctionSynopsisFile(std::string& dir, ScriptCmd& cmd);
      static void writeHtmlFunctionUsageFile(std::string& dir, ScriptCmd& cmd);
      static void writeHtmlSymbolSynopsisFile(std::string& dir, ScriptCmd& cmd);
      static void writeHtmlSymbolUsageFile(std::string& dir, ScriptCmd& cmd);
      static void writeHtmlDataTypes(Script* sc,  std::string& dir);
      static void writeHtmlDataTypeFiles(std::string& dir, ScriptDataType& type);
      static void writeDataType(std::ofstream& fout, ScriptDataType& type);
      static void writeHtmlDataTypeSynopsisFile(std::string& dir, ScriptDataType& type);
      static void writeHtmlDataTypeUsageFile(std::string& dir, ScriptDataType& type);
      static void writeHtmlDataTypeIndexFile(std::string& dir, ScriptDataType& type);

    private:
    }; // End class HtmlDoc
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_HTMLDOC_H
