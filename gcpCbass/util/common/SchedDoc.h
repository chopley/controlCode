// $Id: SchedDoc.h,v 1.1.1.1 2009/07/06 23:57:26 eml Exp $

#ifndef GCP_UTIL_SCHEDDOC_H
#define GCP_UTIL_SCHEDDOC_H

/**
 * @file SchedDoc.h
 * 
 * Tagged: Tue Jan 27 22:22:15 NZDT 2009
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:26 $
 * 
 * @author username: Command not found.
 */

#include "gcp/util/common/String.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace gcp {
  namespace util {

    class SchedDoc {
    public:

      /**
       * Constructor.
       */
      SchedDoc();

      /**
       * Copy Constructor.
       */
      SchedDoc(const SchedDoc& objToBeCopied);

      /**
       * Copy Constructor.
       */
      SchedDoc(SchedDoc& objToBeCopied);

      /**
       * Const Assignment Operator.
       */
      void operator=(const SchedDoc& objToBeAssigned);

      /**
       * Assignment Operator.
       */
      void operator=(SchedDoc& objToBeAssigned);

      /**
       * Output Operator.
       */
      friend std::ostream& operator<<(std::ostream& os, SchedDoc& obj);

      /**
       * Destructor.
       */
      virtual ~SchedDoc();

      void documentSchedules(std::string schDir, std::string outputDir);

    private:

      void ingestFile(std::string fileName, std::ostringstream& os, std::vector<std::string>& commands);

      std::vector<std::string> generateFileList(std::string schDir);
      void generateDocumentation(std::string schDir, std::string outputDir, std::string schName);
      void createDirs(std::string& dir, std::vector<std::string>& schedules);
      bool lineContainsCommand(std::string& s);
      void searchForCommandDocumentation(String& str, std::vector<std::string>& commands, std::vector<std::string>& commandDocs);

      void createIndexFile(std::string outputDir, std::vector<std::string>& schedules);
      void writeHtmlScheduleList(std::string dir, std::vector<std::string>& fileList);
      void writeHtmlFooter(std::ofstream& fout);
      void writeHtmlHeader(std::ofstream& fout, std::string path);
      void writeHtmlStyleSheet(std::string& dir);
      void writeHtmlScheduleSynopsisFile(std::string& dir, std::string& schedName, std::string& content, std::string& callSequence);
      void writeHtmlScheduleDocumentationFile(std::string& dir, std::string& schedName, std::string& content);

      void searchForCommandDocumentation(String& str, std::vector<std::string>& commands, std::vector<std::string>& commandSums,
					 std::vector<std::string>& commandDocs);

      void writeHtmlScheduleCommandList(std::string dir, std::string& schedName, std::vector<std::string>& commDeclList);

      void writeHtmlScheduleCommandSynopsisFile(std::string& dir, std::string& schedName, std::string& commDecl, std::string& content);

      void writeHtmlScheduleCommandDocumentationFile(std::string& dir, std::string& schedName, std::string& commName, 
						     std::string& content);

      std::vector<std::string> commandDeclsToSortedCommandNames(std::vector<std::string>& commandDecls);

    }; // End class SchedDoc

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_SCHEDDOC_H
