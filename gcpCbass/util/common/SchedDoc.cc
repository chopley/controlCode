
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/FunctionName.h"
#include "gcp/util/common/DirList.h"
#include "gcp/util/common/SchedDoc.h"
#include "gcp/util/common/Sort.h"
#include "gcp/util/common/SpecificName.h"
#include "gcp/util/common/String.h"

#include<iostream>
#include<fstream>

#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
SchedDoc::SchedDoc() {}

/**.......................................................................
 * Const Copy Constructor.
 */
SchedDoc::SchedDoc(const SchedDoc& objToBeCopied)
{
  *this = (SchedDoc&)objToBeCopied;
};

/**.......................................................................
 * Copy Constructor.
 */
SchedDoc::SchedDoc(SchedDoc& objToBeCopied)
{
  *this = objToBeCopied;
};

/**.......................................................................
 * Const Assignment Operator.
 */
void SchedDoc::operator=(const SchedDoc& objToBeAssigned)
{
  *this = (SchedDoc&)objToBeAssigned;
};

/**.......................................................................
 * Assignment Operator.
 */
void SchedDoc::operator=(SchedDoc& objToBeAssigned)
{
  std::cout << "Calling default assignment operator for class: SchedDoc" << std::endl;
};

/**.......................................................................
 * Output Operator.
 */
std::ostream& gcp::util::operator<<(std::ostream& os, SchedDoc& obj)
{
  os << "Default output operator for class: SchedDoc" << std::endl;
  return os;
};

/**.......................................................................
 * Destructor.
 */
SchedDoc::~SchedDoc() {}

/**.......................................................................
 * Read through a directory, generating schdeule documentation for schedule
 * files in it.
 */
void SchedDoc::documentSchedules(std::string schDir, std::string outputDir)
{
  // Generate the list of schedule files

  std::vector<std::string> fileList = generateFileList(schDir);
  fileList = Sort::sort(fileList);

  // Create output dirs

  createDirs(outputDir, fileList);

  writeHtmlStyleSheet(outputDir);

  // Now create the index file

  createIndexFile(outputDir, fileList);

  // And the schedule list
  
  writeHtmlScheduleList(outputDir, fileList);

  // Now loop through files, adding to the index file, and creating
  // individual files for each schedule

  for(unsigned iFile=0; iFile < fileList.size(); iFile++) {
    generateDocumentation(schDir, outputDir, fileList[iFile]);
  }
}

/**.......................................................................
 * Create the top-level index file
 */
void SchedDoc::createIndexFile(std::string outputDir, std::vector<std::string>& schedules)
{
  std::ostringstream os;
  os << outputDir << "/scheduleDocumentation/ScheduleIndex.html";
  std::ofstream fout(os.str().c_str(), ios::out);

  fout << "<html>" << std::endl;
  fout << "<head>" << std::endl;
  fout << "<title>" << SpecificName::experimentNameCaps()
       << " Schedule Documentation</title>" << std::endl;
  fout << "</head>" << std::endl;
  fout << "<frameset cols=\"30%, 70%\" border=\"2\" frameborder=\"yes\""
       << " framespacing=\"0\">" << std::endl;
  fout << "  <frame name=\"left\" src=\"ScheduleList.html\">" << std::endl;
  fout << "  <frameset rows=\"30%, 70%\" border=\"2\" frameborder=\"yes\""
       << "framespacing=\"0\">" << std::endl;
  fout << "    <frame name=\"topRight\"  src=\"schedules/"
       << schedules[0] << "/Synopsis.html\">\n";
  fout << "    <frame name=\"botRight\"  src=\"schedules/"
       << schedules[0] << "/Documentation.html\">\n";
  fout << "  </frameset>" << std::endl;
  fout << "</frameset>" << std::endl;
  fout << "</html>" << std::endl;
}

/**.......................................................................
 * Generate a list of schedule files in a directory
 */
std::vector<string> SchedDoc::generateFileList(std::string schDir)
{
  std::vector<string> fileList;

  DirList dirList(schDir, true);
  std::list<gcp::util::DirList::DirEnt> dirEnt = dirList.getFiles(true);

  std::list<gcp::util::DirList::DirEnt>::iterator entry;
  for(entry = dirEnt.begin(); entry != dirEnt.end(); entry++) {

    if(entry->name_.find(".sch", 0) != std::string::npos && 
       entry->name_.find(".sch~", 0) == std::string::npos &&
       entry->name_.find(".sch.", 0) == std::string::npos) {
      fileList.push_back(entry->name_);
    }
  }

  return fileList;
}

/**.......................................................................
 * Utility function to read a file into a string stream
 */
void SchedDoc::ingestFile(std::string fileName, ostringstream& os, std::vector<std::string>& commands)
{
  std::ifstream fin(fileName.c_str());

  string s;
  String str;

  ostringstream comDef;

  while(getline(fin, s)) {
    os << s << std::endl;

    // Look for command definitions

    if(lineContainsCommand(s)) {
      comDef.str("");
      str = s;
      comDef << "command " << str.findNextInstanceOf("command ", true, ")", true, false).str() << ")";
      commands.push_back(comDef.str());
    }

  }

  fin.close();
}

/**.......................................................................
 * Return true if the line contains a command definition
 */
bool SchedDoc::lineContainsCommand(std::string& s)
{
  String str = s;

  // Search for the strings we expect if this is a command definition

  if(str.contains("command ") && str.contains('(') && str.contains(')')) {

    std::string::size_type istart = s.find("command ", 0);

    // Make sure no comment tag precedes the occurrence of 'command'

    char c;
    for(std::string::size_type i=0; i < istart; i++) {
      if(s[i] == '#')
	return false;
    }

    // This means we found the string 'command ' anda it wasn't
    // preceded by a comment tag -- this is a real command def.

    return true;

  } else {
    return false;
  }
}

/**.......................................................................
 * Method to generate documentation for a single file
 */
void SchedDoc::generateDocumentation(std::string schDir, std::string outputDir, std::string schName)
{
  std::ostringstream fileName;
  std::ostringstream os;
  std::vector<std::string> commandDecls;
  std::vector<std::string> commandSums;
  std::vector<std::string> commandDocs;

  fileName << schDir << "/" << schName;
  ingestFile(fileName.str(), os, commandDecls);

  String str(os.str());

  // Look for command documentation

  searchForCommandDocumentation(str, commandDecls, commandSums, commandDocs);

  // See if this schedule has argumments

  str.resetToBeginning();
  String callSequence;
  if(str[0] == '(') {
    callSequence = str.findNextInstanceOf("(", true, ")", true, true);
  }

  str.resetToBeginning();
  String doc = str.findNextInstanceOf("#SSCH_DOC", true, "#ESCH_DOC", true, true);

  str.resetToBeginning();
  String sum = str.findNextInstanceOf("#SSCH_SUM", true, "#ESCH_SUM", true, true);

  writeHtmlScheduleSynopsisFile(outputDir, schName, sum.str(), callSequence.str());
  writeHtmlScheduleDocumentationFile(outputDir, schName, doc.str());

  std::vector<std::string> commandNames = commandDeclsToSortedCommandNames(commandDecls);
  writeHtmlScheduleCommandList(outputDir, schName, commandNames);

  for(unsigned i=0; i < commandDecls.size(); i++) {
    writeHtmlScheduleCommandSynopsisFile(outputDir, schName, commandDecls[i], commandSums[i]);
    writeHtmlScheduleCommandDocumentationFile(outputDir, schName, commandDecls[i], commandDocs[i]); 
  }

}

void SchedDoc::createDirs(std::string& dir, std::vector<std::string>& schedules)
{
  std::ostringstream os;

  os << dir << "/scheduleDocumentation";
  mkdir(os.str().c_str(), 0755);

  os.str("");
  os << dir << "/scheduleDocumentation/schedules";
  mkdir(os.str().c_str(), 0755);

  // Now create a directory for each schedule under the main directory

  for(unsigned i=0; i < schedules.size(); i++) {
    os.str("");
    os << dir << "/scheduleDocumentation/schedules/" << schedules[i];
    mkdir(os.str().c_str(), 0755);

    // And create a subdirectory for commands of this schedule

    os << "/commands";
    mkdir(os.str().c_str(), 0755);
  }
}

void SchedDoc::writeHtmlScheduleList(std::string dir, std::vector<std::string>& fileList)
{
  std::ostringstream os;
  os << dir << "/scheduleDocumentation/ScheduleList.html";
  std::ofstream fout(os.str().c_str(), ios::out);
  
  writeHtmlHeader(fout, "./");
  
  // Write the Java load function                                                                             
  
  fout << "<script language=JavaScript>" << std::endl;
  fout << "function loadPage(top, bot){" << std::endl;
  fout << "  parent.topRight.location.href=top;" << std::endl;
  fout << "  parent.botRight.location.href=bot;" << std::endl;
  fout << "}" << std::endl;
  fout << "</script>" << std::endl;
  fout << std::endl;
  fout << std::endl;
  fout << "<script language=JavaScript>" << std::endl;
  fout << "function loadAll(left, top, bot){" << std::endl;
  fout << "  parent.left.location.href=left;" << std::endl;
  fout << "  parent.topRight.location.href=top;" << std::endl;
  fout << "  parent.botRight.location.href=bot;" << std::endl;
  fout << "}" << std::endl;
  fout << "</script>" << std::endl;
  fout << std::endl;
  
  // Now write out documentation for built-in commands                                                        
  
  fout << "<a name=\"schList\"></a>" << std::endl;
  fout << "<h2>" << SpecificName::experimentNameCaps()
       << " Schedule List" << "</h2>\n" << std::endl;
  fout << "<hr>" << std::endl;

  fout << "<dl>" << std::endl;

  // Write out entries for schedule files

  for(unsigned i=0; i < fileList.size(); i++) {
    
    fout << "<dt><a class=plain href=\"javascript:loadPage('schedules/"
         << fileList[i] << "/Synopsis.html', 'schedules/"
         << fileList[i] << "/Documentation.html')\">"
         << fileList[i] << "</a>" << std::endl; 

    fout << "<ul>" << std::endl
	 << "<li><a class=plain href=\"javascript:loadAll('schedules/"
         << fileList[i] << "/CommandList.html', 'schedules/"
         << fileList[i] << "/Synopsis.html', 'schedules/"
         << fileList[i] << "/Documentation.html')\">"
         << "commands" << "</a></li>"
	 << "</ul></dt>" << std::endl;
  }
  
  writeHtmlFooter(fout);
  
  fout << "  </body>" << std::endl;
  fout << "</html>" << std::endl;
  
  fout.close();
}

void SchedDoc::writeHtmlScheduleCommandList(std::string dir, std::string& schedName, std::vector<std::string>& commNames)
{
  std::ostringstream os;
  os << dir << "/scheduleDocumentation/schedules/" << schedName << "/CommandList.html";
  std::ofstream fout(os.str().c_str(), ios::out);
  
  writeHtmlHeader(fout, "../../");
  
  // Write the Java load function                                                                             
  
  fout << "<script language=JavaScript>" << std::endl;
  fout << "function loadPage(top, bot){" << std::endl;
  fout << "  parent.topRight.location.href=top;" << std::endl;
  fout << "  parent.botRight.location.href=bot;" << std::endl;
  fout << "}" << std::endl;
  fout << "</script>" << std::endl;
  fout << std::endl;
  fout << std::endl;
  fout << "<script language=JavaScript>" << std::endl;
  fout << "function loadAll(left, top, bot){" << std::endl;
  fout << "  parent.left.location.href=left;" << std::endl;
  fout << "  parent.topRight.location.href=top;" << std::endl;
  fout << "  parent.botRight.location.href=bot;" << std::endl;
  fout << "}" << std::endl;
  fout << "</script>" << std::endl;
  fout << std::endl;
  
  // Now write out documentation for built-in commands                                                        
  
  fout << "<a name=\"schList\"></a>" << std::endl;
  fout << "<h2>" 
       << "Command List for the " 
       << "<a class=plain href=\"javascript:loadAll('../../ScheduleList.html', 'Synopsis.html', 'Documentation.html')\">"
       << schedName << "</a> Schedule</h2>\n" << std::endl;
  fout << "<hr>"  << std::endl;

  fout << "<dl>" << std::endl;

  // Write out entries for command files

  for(unsigned i=0; i < commNames.size(); i++) {
    
    fout << "<dt><a class=plain href=\"javascript:loadPage('commands/"
         << commNames[i] << "_Synopsis.html', 'commands/"
         << commNames[i] << "_Documentation.html')\">"
         << commNames[i] << "</a></dt>" << std::endl;
  }
  
  writeHtmlFooter(fout);
  
  fout << "</body>" << std::endl;
  fout << "</html>" << std::endl;
  
  fout.close();
}

void SchedDoc::writeHtmlFooter(std::ofstream& fout)
{
  gcp::util::TimeVal tVal;
  tVal.setToCurrentTime();
  gcp::util::FunctionName fn(__PRETTY_FUNCTION__);

  fout << "<hr>"  << std::endl;
  fout << "<i>Last updated on " << tVal << " by the " << SpecificName::experimentName() << "SchedDoc" << " command</i>\n";
}

void SchedDoc::writeHtmlHeader(std::ofstream& fout, std::string path)
{
  fout << "<html>" << std::endl;
  fout << "<head>" << std::endl;
  fout << "<title>" << SpecificName::experimentNameCaps()
       << " Command List" << "</title>" << std::endl;
  fout << "<link rel=stylesheet href=\"" << path << "StyleSheet.css\" type=\"text/css\">" << std::endl;
  fout << "</head>" << std::endl;
  fout << std::endl;
  fout << "<body bgcolor=\"#add8e6\">" << std::endl;
  fout << "<font face=\"Verdana, Arial, Helvetica, sans-serif\""
       << "size=\"2\" color=\"#000000\">" << std::endl;
}

void SchedDoc::writeHtmlStyleSheet(std::string& dir)
{
  std::ostringstream os;
  os << dir << "/scheduleDocumentation/StyleSheet.css";
  std::ofstream fout(os.str().c_str(), ios::out);
  
  fout << ".declaration {" << std::endl;
  fout << "   color: #000080;" << std::endl;
  fout << "}" << std::endl;
  fout << std::endl;
  fout << ".dataType {" << std::endl;
  fout << "  color: #000080;" << std::endl;
  fout << "}" << std::endl;
  fout << std::endl;
  fout << ".schedule {" << std::endl;
  fout << "  color: #000080;" << std::endl;
  fout << "}" << std::endl;
  fout << std::endl;
  fout << ".function {" << std::endl;
  fout << "  color: #000080;" << std::endl;
  fout << "}" << std::endl;
  fout << std::endl;
  fout << ".enum {" << std::endl;
  fout << "  color: #000080;" << std::endl;
  fout << "  font-weight:bold;" << std::endl;
  fout << "  font-style:italic;" << std::endl;
  fout << "}" << std::endl;
  fout << std::endl;

  fout << "a:link.plain {" << std::endl;
  fout << "  color: #000080;" << std::endl;
  fout << "  text-decoration: none;" << std::endl;
  fout << "  font-weight:normal;" << std::endl;
  fout << "}" << std::endl;
  fout << std::endl;

  fout << "a:visited.plain {" << std::endl;
  fout << "  color: #000080;" << std::endl;
  fout << "  text-decoration: none;" << std::endl;
  fout << "  font-weight:normal;" << std::endl;
  fout << "}" << std::endl;
  fout << std::endl;

  fout << "a:active.plain {" << std::endl;
  fout << "  color: #000080;" << std::endl;
  fout << "  text-decoration: none;" << std::endl;
  fout << "  font-weight:normal;" << std::endl;
  fout << "}" << std::endl;
  fout << std::endl;

  fout << "a:focus.plain {" << std::endl;
  fout << "  color: #000080;" << std::endl;
  fout << "  text-decoration: none;" << std::endl;
  fout << "  font-weight:normal;" << std::endl;
  fout << "}" << std::endl;
  fout << std::endl;

  fout << "a:hover.plain {" << std::endl;
  fout << "  text-decoration: none;" << std::endl;
  fout << "  font-weight:bold;" << std::endl;
  fout << "}" << std::endl;
  fout << std::endl;

  fout << ".code {" << std::endl;
  fout << "  color: blue;"      << std::endl;
  fout << "  font-weight:bold;" << std::endl;
  fout << "  font-size:10pt;"   << std::endl;
  fout << "}" << std::endl;
  fout << std::endl;
  
  fout.close();
}

void SchedDoc::writeHtmlScheduleSynopsisFile(std::string& dir, std::string& schedName, std::string& content, std::string& callSequence)
{
  std::ostringstream os;
  os << dir << "/scheduleDocumentation/schedules/" << schedName << "/Synopsis.html";
  std::ofstream fout(os.str().c_str(), ios::out);
  
  writeHtmlHeader(fout, "../../");
  
  fout << "<script language=JavaScript>" << std::endl;
  fout << "function loadPage(bot){" << std::endl;
  fout << "  parent.botRight.location.href=bot;" << std::endl;
  fout << "}" << std::endl;
  fout << "</script>" << std::endl;
  fout << std::endl;
  
  fout << "<h2>The <span class=schedule>" << schedName
       << "</span> schedule (synopsis)" << "</h2>\n<hr>" << std::endl;
  
  if(callSequence.size() > 0) {
    fout << std::endl << "<p>" << std::endl;
    fout << "<dt> Call like: </dt>" << std::endl;
    fout << "<dd><span class=code>" << schedName << "(" << callSequence << ") ";
    fout << "</span></dd></dl><hr>";
  }
  
  String::replace(content, '#', ' ');
  fout << "<dd>" << "<pre>" << content << "</pre>" << "</dd>" << std::endl;


  fout << "</body>";
  fout << "</html>";
  
  fout.close();
}

void SchedDoc::writeHtmlScheduleDocumentationFile(std::string& dir, std::string& schedName, std::string& content)
{
  std::ostringstream os;
  os << dir << "/scheduleDocumentation/schedules/" << schedName << "/Documentation.html";
  std::ofstream fout(os.str().c_str(), ios::out);
  
  writeHtmlHeader(fout, "../../");
  
  fout << "<script language=JavaScript>" << std::endl;
  fout << "function loadPage(bot){" << std::endl;
  fout << "  parent.topRight.location.href=bot;" << std::endl;
  fout << "}" << std::endl;
  fout << "</script>" << std::endl;
  fout << std::endl;
  
  fout << "<h2>The <span class=schedule>" << schedName
       << "</span> schedule (documentation)" << "</h2>\n<hr>" << std::endl;
  
  String::replace(content, '#', ' ');

  fout << "<dd>" << "<pre>" << content << "</pre>" << "</dd>" << std::endl;
  
  fout << "</body>";
  fout << "</html>";
  
  fout.close();
}

void SchedDoc::writeHtmlScheduleCommandSynopsisFile(std::string& dir, std::string& schedName, 
						    std::string& commDecl, std::string& content)
{
  String command = commDecl;
  String commName = command.findNextInstanceOf("command ", true, "(", true, true);
  commName.strip(" ");

  command.resetToBeginning();
  String commCallSequence = command.findNextInstanceOf("(", true, ")", true, true);

  std::ostringstream os;
  os << dir << "/scheduleDocumentation/schedules/" << schedName << "/commands/" << commName.str() << "_Synopsis.html";
  std::ofstream fout(os.str().c_str(), ios::out);
  
  writeHtmlHeader(fout, "../../../");
  
  fout << "<script language=JavaScript>" << std::endl;
  fout << "function loadPage(bot){" << std::endl;
  fout << "  parent.botRight.location.href=bot;" << std::endl;
  fout << "}" << std::endl;
  fout << "</script>" << std::endl;
  fout << std::endl;
  
  fout << "<h2>The " << schedName << "::" << "<span class=schedule>" << commName.str()
       << "</span> command (synopsis)" << "</h2>\n<hr>" << std::endl;
  
  fout << std::endl << "<p>" << std::endl;
  fout << "<dt> Declaration: </dt>" << std::endl;
  fout << "<dd><span class=code>" << commName.str() << "(" << commCallSequence.str() << ") ";
  fout << "</span></dd></dl><hr>";

  String::replace(content, '#', ' ');
  fout << "<dd>" << "<pre>" << content << "</pre>" << "</dd>" << std::endl;

  
  fout << "</body>";
  fout << "</html>";
  
  fout.close();
}

void SchedDoc::writeHtmlScheduleCommandDocumentationFile(std::string& dir, std::string& schedName, 
							 std::string& commDecl, std::string& content)
{
  String command = commDecl;
  String commName = command.findNextInstanceOf("command ", true, "(", true, true);
  commName.strip(" ");

  std::ostringstream os;
  os << dir << "/scheduleDocumentation/schedules/" << schedName << "/commands/" << commName.str() << "_Documentation.html";
  std::ofstream fout(os.str().c_str(), ios::out);
  
  writeHtmlHeader(fout, "../../../");
  
  fout << "<script language=JavaScript>" << std::endl;
  fout << "function loadPage(bot){" << std::endl;
  fout << "  parent.topRight.location.href=bot;" << std::endl;
  fout << "}" << std::endl;
  fout << "</script>" << std::endl;
  fout << std::endl;
  
  fout << "<h2>The " << schedName << "::" << "<span class=schedule>" << commName.str()
       << "</span> command (documentation)" << "</h2>\n<hr>" << std::endl;

  String::replace(content, '#', ' ');

  fout << "<dd>" << "<pre>" << content << "</pre>" << "</dd>" << std::endl;
  
  fout << "</body>";
  fout << "</html>";
  
  fout.close();
}

void SchedDoc::searchForCommandDocumentation(String& str, std::vector<std::string>& commands, std::vector<std::string>& commandSums,
					     std::vector<std::string>& commandDocs) 
{
  String comDoc;
  commandSums.resize(commands.size());
  commandDocs.resize(commands.size());

  str.resetToBeginning();
  for(unsigned i=0; i < commands.size(); i++) {
    
    if(i==0) {
      
      // If this is the first command, search from the beginning of the file to the command definition
      
      comDoc = str.findNextInstanceOf("", false, commands[i], true, false);

      commandSums[i] = comDoc.findNextInstanceOf("#SCOM_SUM", true, "#ECOM_SUM", true, true).str();
      comDoc.resetToBeginning();
      commandDocs[i] = comDoc.findNextInstanceOf("#SCOM_DOC", true, "#ECOM_DOC", true, true).str();
      
    } else {
      
      // Else search from the last command definition to the current command definition
      
      comDoc = str.findNextInstanceOf(commands[i-1], true, commands[i], true, false);

      commandDocs[i] = comDoc.findNextInstanceOf("#SCOM_DOC", true, "#ECOM_DOC", true, true).str();
      comDoc.resetToBeginning();
      commandDocs[i] = comDoc.findNextInstanceOf("#SCOM_DOC", true, "#ECOM_DOC", true, true).str();
    }
    
  }
  str.resetToBeginning();
}

std::vector<std::string> SchedDoc::commandDeclsToSortedCommandNames(std::vector<std::string>& commandDecls)
{
  std::vector<std::string> commandNames;

  commandNames.resize(commandDecls.size());

  String decl, name;
  for(unsigned i=0; i < commandDecls.size(); i++) {
    decl = commandDecls[i];
    name = decl.findNextInstanceOf("command ", true, "(", true, true);
    name.strip(" ");
    commandNames[i] = name.str();
  }

  commandNames = Sort::sort(commandNames);

  return commandNames;
}
