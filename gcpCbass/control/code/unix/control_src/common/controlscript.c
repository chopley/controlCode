#include "genericcontrol.h"
#include "controlscript.h"
#include "genericscript.h"
#include "genericscheduler.h"

#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>

#include <sys/stat.h>
#include <sys/types.h>

#include "gcp/util/common/SpecificName.h"

using namespace std;
using namespace gcp::util;

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

/*
 * Define constructor and destructor functions for schedule-specific
 * project data.
 */
static SC_NEW_FN(new_sch_data);
static SC_CLR_FN(clr_sch_data);
static SC_DEL_FN(del_sch_data);

/*.......................................................................
 * This is the constructor for a schedule-specific data object.
 */
static SC_NEW_FN(new_sch_data)
{
/*
 * Attempt to allocate the data object.
 */
  ScheduleData *data = (ScheduleData* )malloc(sizeof(ScheduleData));
  if(!data) {
    lprintf(stderr, "Insufficient memory to allocate schedule data.\n");
    return NULL;
  };
/*
 * Before attempting any operation that might fail, initialize the container
 * at least up to the point at which it can be safely passed to del_sch_data().
 */
  data->ref_count = 1;
  data->schedules = new_ScriptList(sc);
  if(!data->schedules) {
    lprintf(stderr,
	    "Insufficient memory to allocate list of script schedules.\n");
    return del_sch_data(sc, data);
  };
  return data;
}

/*.......................................................................
 * Garbage collect any non-script objects allocated during compilation
 * of a schedule.
 */
static SC_CLR_FN(clr_sch_data)
{
/*
 * Get the resource object of the parent thread.
 */
  Scheduler *sch = (Scheduler* )cp_ThreadData((ControlProg *)sc->project, 
					      CP_SCHEDULER);
/*
 * Get the script-specific data.
 */
  ScheduleData *sd = (ScheduleData* )data;
/*
 * Discard any schedules that aren't currently queued to be run.
 */
  while(sd->schedules->head) {
    Script *sched = (Script* )del_ListNode(sd->schedules, sd->schedules->head, 
					   NULL);
    sch_discard_schedule(sch, sched);
  };
  return 0;
}

/*.......................................................................
 * This is the destructor for a schedule-specific data object.
 */
static SC_DEL_FN(del_sch_data)
{
  if(data) {
    ScheduleData *sd = (ScheduleData* )data;
/*
 * Delete objects that were allocated while compiling the script.
 */
    clr_sch_data(sc, data);
    sd->schedules = NULL;  /* The list will be deleted by del_Script() */
    free(data);
  };
  return NULL;
}

/*.......................................................................
 * Create a new scripting environment.
 *
 * Input:
 *  cp    ControlProg *  The host control program.
 *  batch         int    True to create an environment for schedulable
 *                       scripts. False to create an environment for
 *                       single-line interactive commands.
 *  signals HashTable *  The symbol table of signals maintained by
 *                       the scheduler.
 * Output:
 *  return     Script *  The new object, or NULL on error.
 */
Script *new_ControlScript(ControlProg *cp, int batch, HashTable *signals)
{
  Script *sc=NULL;         // The object to be returned 
  
  // Create an empty script environment.
  
  sc = new_Script(cp, new_sch_data, clr_sch_data, del_sch_data, signals);

  if(!sc)
    return NULL;
  
  sc = add_GenericScriptCommands(cp, sc, batch, signals);
  sc = add_SpecificScriptCommands(cp, sc, batch, signals);

  return sc;
}

/*.......................................................................
 * Report an error if a command destined for the real-time controller
 * is sent when no controller is connected.
 *
 * Input:
 *  sc          Script *  The host scripting environment.
 *  cmd           char *  The name of the command that was about
 *                        to be executed.
 * Output:
 *  return         int    0 - The controller is connected.
 *                        1 - The controller is not connected.
 */
int rtc_offline(Script *sc, char *cmd)
  {
  if(!cp_rtc_online((ControlProg* )sc->project)) {
    lprintf(stderr,
	    "The \"%s\" command was dropped. The controller is offline.\n",
	    cmd);
  };
  return 0;
}

void generateAutoDocumentation(Script* sc, std::string dir)
{
  createDirs(dir);
  writeHtmlStyleSheet(sc, dir);
  writeHtmlCommandIndexFile(sc, dir);
  writeHtmlCommandList(sc, dir);
  writeHtmlDataTypes(sc, dir);
}

static void createDirs(std::string& dir)
{
  std::ostringstream os;
  os << dir << "/commands";
  mkdir(os.str().c_str(), 0755);
  os.str("");
  os << dir << "/dataTypes";
  mkdir(os.str().c_str(), 0755);
  os.str("");
}

static void writeHtmlStyleSheet(Script* sc, std::string& dir)
{
  std::ostringstream os;
  os << dir << "/StyleSheet.css";
  std::ofstream fout(os.str().c_str(), ios::out);

  fout << ".declaration {" << std::endl;
  fout << "   color: #000080;" << std::endl;
  fout << "}" << std::endl;
  fout << std::endl;
  fout << ".dataType {" << std::endl;
  fout << "  color: #000080;" << std::endl; 
  fout << "}" << std::endl;
  fout << std::endl;
  fout << ".command {" << std::endl;
  fout << "  color: #000080;" << std::endl; 
  fout << "}" << std::endl;
  fout << std::endl;
  fout << ".function {" << std::endl;
  fout << "  color: #000080;" << std::endl; 
  fout << "}" << std::endl;
  fout << std::endl;
  fout << ".symbol {" << std::endl;
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
  fout << "  color: blue;" << std::endl;
  fout << "  font-weight:bold;" << std::endl;
  fout << "}" << std::endl;
  fout << std::endl;

  fout.close();
}

static void writeHtmlCommandIndexFile(Script* sc, std::string& dir)
{
  std::ostringstream os;
  os << dir << "/CommandIndex.html";
  std::ofstream fout(os.str().c_str(), ios::out);

  sc->commands_->sort();

  fout << "<html>" << std::endl;
  fout << "<head>" << std::endl;
  fout << "<title>" << gcp::util::SpecificName::experimentNameCaps() 
       << " Command Documentation</title>" << std::endl;
  fout << "</head>" << std::endl;
  fout << "<frameset cols=\"30%, 70%\" border=\"2\" frameborder=\"yes\""
       << " framespacing=\"0\">" << std::endl;
  fout << "  <frame name=\"left\" src=\"CommandList.html\">" << std::endl;
  fout << "  <frameset rows=\"20%, 80%\" border=\"2\" frameborder=\"yes\""
       << "framespacing=\"0\">" << std::endl;
  fout << "    <frame name=\"topRight\"  src=\"commands/" 
       << sc->commands_->front().name_ << "_Synopsis.html\">\n";
  fout << "    <frame name=\"botRight\"  src=\"commands/"
       << sc->commands_->front().name_ << ".html\">\n";
  fout << "  </frameset>" << std::endl;
  fout << "</frameset>" << std::endl;
  fout << "</html>" << std::endl;

  fout.close();
}

static void writeHtmlCommandList(Script* sc, std::string& dir)
{
  sc->commands_->sort();
  sc->functions_->sort();
  sc->symbols_->sort();

  std::ostringstream os;
  os << dir << "/CommandList.html";
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

  // Now write out documentation for built-in commands

  fout << "<a name=\"cmdList\"></a>" << std::endl;
  fout << "<h1>" << SpecificName::experimentNameCaps() 
       << " Command List" << "</h1>\n" << std::endl;
  fout << "<a class=plain href=\"#fnList\">" << "to function list</a><br>" << std::endl;
  fout << "<a class=plain href=\"#symList\">" << "to symbol list</a><hr>" << std::endl;

  fout << "<dl>" << std::endl;
  for(std::list<ScriptCmd>::iterator iCmd=sc->commands_->begin(); 
      iCmd != sc->commands_->end(); iCmd++) {

    fout << "<dt><a class=plain href=\"javascript:loadPage('commands/" 
	 << iCmd->name_ << "_Synopsis.html', 'commands/"
	 << iCmd->name_ << ".html')\">"
	 << iCmd->name_ << "</a></dt>" << std::endl;

    writeHtmlCommandSynopsisFile(dir, *iCmd);
    writeHtmlCommandUsageFile(dir, *iCmd);
  }

  // Now write out documentation for built-in functions

  fout << "<a name=\"fnList\"></a>" << std::endl;
  fout << "<hr><h1>" << SpecificName::experimentNameCaps() 
       << " Function List" << "</h1>\n" << std::endl;
  fout << "<a class=plain href=\"#cmdList\">" << "back to command list</a><br>" << std::endl;
  fout << "<a class=plain href=\"#symList\">" << "to symbol list</a><hr>" << std::endl;

  for(std::list<ScriptCmd>::iterator iCmd=sc->functions_->begin(); 
      iCmd != sc->functions_->end(); iCmd++) {

    fout << "<dt><a class=plain href=\"javascript:loadPage('commands/" 
	 << iCmd->name_ << "_Synopsis.html', 'commands/"
	 << iCmd->name_ << ".html')\">"
	 << iCmd->name_ << "</a></dt>" << std::endl;

    writeHtmlFunctionSynopsisFile(dir, *iCmd);
    writeHtmlFunctionUsageFile(dir, *iCmd);
  }

  // Now write out documentation for script symbols

  fout << "<a name=\"symList\"></a>" << std::endl;
  fout << "<hr><h1>" << SpecificName::experimentNameCaps() 
       << " Script Symbols" << "</h1>\n" << std::endl;
  fout << "<a class=plain href=\"#cmdList\">" << "back to command list</a><br>" << std::endl;
  fout << "<a class=plain href=\"#fnList\">"  << "back to function list</a><hr>" << std::endl;

  for(std::list<ScriptCmd>::iterator iCmd=sc->symbols_->begin(); 
      iCmd != sc->symbols_->end(); iCmd++) {

    fout << "<dt><a class=plain href=\"javascript:loadPage('commands/" 
	 << iCmd->name_ << "_Synopsis.html', 'commands/"
	 << iCmd->name_ << ".html')\">"
	 << iCmd->name_ << "</a></dt>" << std::endl;

    writeHtmlSymbolSynopsisFile(dir, *iCmd);
    writeHtmlSymbolUsageFile(dir, *iCmd);
  }

  fout << "</dl>" << std::endl;

  writeHtmlFooter(fout);

  fout << "  </body>" << std::endl;
  fout << "</html>" << std::endl;

  fout.close();
}

static void writeHtmlFooter(std::ofstream& fout)
{
  gcp::util::TimeVal tVal;
  tVal.setToCurrentTime();

  fout << "<hr>"  << std::endl;
  fout << "<i>Last updated on " << tVal << " by the autoDoc() command</i>\n";
}

static void writeHtmlHeader(std::ofstream& fout, std::string path)
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

static void writeHtmlCommandSynopsisFile(std::string& dir, ScriptCmd& cmd)
{
  std::ostringstream os;
  os << dir << "/commands/" << cmd.name_ << "_Synopsis.html";
  std::ofstream fout(os.str().c_str(), ios::out);

  writeHtmlHeader(fout, "../");

  fout << "<script language=JavaScript>" << std::endl;
  fout << "function loadPage(bot){" << std::endl;
  fout << "  parent.botRight.location.href=bot;" << std::endl;
  fout << "}" << std::endl;
  fout << "</script>" << std::endl;
  fout << std::endl;

  fout << "<h2>The <span class=command>" << cmd.name_ 
  << "</span> command (synopsis)" << "</h2>\n<hr>" << std::endl;

  fout << "<dl>" << std::endl;
  fout << "<dt><span class=declaration><a class=plain "
       << "href=\"javascript:loadPage('" << cmd.name_ << ".html')\">"
       << cmd.name_ << "</a>(";

  bool hadOptArgs=false;
  for(std::list<CmdArg>::iterator iArg=cmd.argList_.begin(); 
      iArg != cmd.argList_.end(); iArg++) {

    if(iArg != cmd.argList_.begin())
      fout << ", ";

    if(iArg->isOptional_ && !hadOptArgs) {
      fout << "[";
      hadOptArgs = true;
    }

    fout << "<a class=plain href=\"javascript:loadPage('../dataTypes/" 
	 << iArg->dataTypeName_ << "_Index.html')\">" 
	 << iArg->dataTypeName_ << "</a> " << iArg->varName_;
  }

  if(hadOptArgs)
    fout << "]";

  fout << ")" << "</span></dt>" << std::endl;
  fout << "<dd>" << cmd.description_ << "</dd>" << std::endl;

  fout << "  </body>";
  fout << "</html>";

  fout.close();
}

static void writeHtmlFunctionSynopsisFile(std::string& dir, ScriptCmd& cmd)
{
  std::ostringstream os;
  os << dir << "/commands/" << cmd.name_ << "_Synopsis.html";
  std::ofstream fout(os.str().c_str(), ios::out);

  writeHtmlHeader(fout, "../");

  fout << "<script language=JavaScript>" << std::endl;
  fout << "function loadPage(bot){" << std::endl;
  fout << "  parent.botRight.location.href=bot;" << std::endl;
  fout << "}" << std::endl;
  fout << "</script>" << std::endl;
  fout << std::endl;

  fout << "<h2>The <span class=function>" << cmd.name_ 
  << "</span> function (synopsis)" << "</h2>\n<hr>" << std::endl;

  fout << "<dl>" << std::endl;
  fout << "<dt><span class=declaration>"
       << "<a class=plain href=\"javascript:loadPage('../dataTypes/" << cmd.retType_.dataTypeName_ << "_Index.html')\">" 
       << cmd.retType_.dataTypeName_ 
       << "</a> <a class=plain href=\"javascript:loadPage('" << cmd.name_ << ".html')\">"
       << cmd.name_ << "</a>(";

  bool hadOptArgs=false;
  for(std::list<CmdArg>::iterator iArg=cmd.argList_.begin(); 
      iArg != cmd.argList_.end(); iArg++) {

    if(iArg != cmd.argList_.begin())
      fout << ", ";

    if(iArg->isOptional_ && !hadOptArgs) {
      fout << "[";
      hadOptArgs = true;
    }

    fout << "<a class=plain href=\"javascript:loadPage('../dataTypes/" 
	 << iArg->dataTypeName_ << "_Index.html')\">" 
	 << iArg->dataTypeName_ << "</a> " << iArg->varName_;
  }

  if(hadOptArgs)
    fout << "]";

  fout << ")" << "</span></dt>" << std::endl;
  fout << "<dd>" << cmd.description_ << "</dd>" << std::endl;
  fout << "</dl>" << std::endl;

  fout << "</body>";
  fout << "</html>";

  writeHtmlFooter(fout);

  fout.close();
}

static void writeHtmlSymbolSynopsisFile(std::string& dir, ScriptCmd& cmd)
{
  std::ostringstream os;
  os << dir << "/commands/" << cmd.name_ << "_Synopsis.html";
  std::ofstream fout(os.str().c_str(), ios::out);

  writeHtmlHeader(fout, "../");

  fout << "<script language=JavaScript>" << std::endl;
  fout << "function loadPage(bot){" << std::endl;
  fout << "  parent.botRight.location.href=bot;" << std::endl;
  fout << "}" << std::endl;
  fout << "</script>" << std::endl;
  fout << std::endl;

  fout << "<h2>The <span class=symbol>" << cmd.name_ 
  << "</span> symbol (synopsis)" << "</h2>\n<hr>" << std::endl;

  fout << "<dl>" << std::endl;
  fout << "<dt><span class=declaration>"
       << "<a class=plain href=\"javascript:loadPage('../dataTypes/" << cmd.retType_.dataTypeName_ << "_Index.html')\">" 
       << cmd.retType_.dataTypeName_ 
       << "</a> <a class=plain href=\"javascript:loadPage('" << cmd.name_ << ".html')\">"
       << cmd.name_ << "</a>(";

  bool hadOptArgs=false;
  for(std::list<CmdArg>::iterator iArg=cmd.argList_.begin(); 
      iArg != cmd.argList_.end(); iArg++) {

    if(iArg != cmd.argList_.begin())
      fout << ", ";

    if(iArg->isOptional_ && !hadOptArgs) {
      fout << "[";
      hadOptArgs = true;
    }

    fout << "<a class=plain href=\"javascript:loadPage('../dataTypes/" 
	 << iArg->dataTypeName_ << "_Index.html')\">" 
	 << iArg->dataTypeName_ << "</a> " << iArg->varName_;
  }

  if(hadOptArgs)
    fout << "]";

  fout << ")" << "</span></dt>" << std::endl;
  fout << "<dd>" << cmd.description_ << "</dd>" << std::endl;
  fout << "</dl>" << std::endl;

  fout << "</body>";
  fout << "</html>";

  writeHtmlFooter(fout);

  fout.close();
}

static void writeHtmlCommandUsageFile(std::string& dir, ScriptCmd& cmd)
{
  std::ostringstream os ;
  os << dir << "/commands/" << cmd.name_ << ".html";

  // Check if the file already exists -- if it does, don't create it!

  std::ifstream fin;
  fin.open(os.str().c_str(), ios::in);

  if(fin) {
    fin.close();
    return;
  }

  // Else just create a stub

  std::ofstream fout(os.str().c_str(), ios::out);

  writeHtmlHeader(fout, "../");

  fout << "<h2>The <span class=command>" << cmd.name_ 
  << "</span> command (usage)" << "</h2>\n<hr>" << std::endl;

  fout << std::endl << std::endl;

  fout << "<h2><i>Context:</i></h2><p>\n\n";
  fout << "<h2><i>Examples:</i></h2><p>\n\n";

  fout << std::endl << std::endl;

  fout << "</body>";
  fout << "</html>";
  fout.close();
}

static void writeHtmlFunctionUsageFile(std::string& dir, ScriptCmd& cmd)
{
  std::ostringstream os ;
  os << dir << "/commands/" << cmd.name_ << ".html";

  // Check if the file already exists -- if it does, don't create it!

  std::ifstream fin;
  fin.open(os.str().c_str(), ios::in);

  if(fin) {
    fin.close();
    return;
  }

  // Else just create a stub

  std::ofstream fout(os.str().c_str(), ios::out);

  writeHtmlHeader(fout, "../");

  fout << "<h2>The <span class=function>" << cmd.name_ 
  << "</span> function (usage)" << "</h2>\n<hr>" << std::endl;

  fout << "  </body>";
  fout << "</html>";

  fout.close();
}

static void writeHtmlSymbolUsageFile(std::string& dir, ScriptCmd& cmd)
{
  std::ostringstream os ;
  os << dir << "/commands/" << cmd.name_ << ".html";

  // Check if the file already exists -- if it does, don't create it!

  std::ifstream fin;
  fin.open(os.str().c_str(), ios::in);

  if(fin) {
    fin.close();
    return;
  }

  // Else just create a stub

  std::ofstream fout(os.str().c_str(), ios::out);

  writeHtmlHeader(fout, "../");

  fout << "<h2>The <span class=symbol>" << cmd.name_ 
  << "</span> symbol (usage)" << "</h2>\n<hr>" << std::endl;

  fout << "  </body>";
  fout << "</html>";

  fout.close();
}

/**.......................................................................
 * Write out documentation about all known data types
 */
static void writeHtmlDataTypes(Script* sc,  std::string& dir)
{
  // First sort the list of data types, then prune any redundant
  // entries from the list

  sc->dataTypes_->sort();
  sc->dataTypes_->unique();
  
  for(std::list<ScriptDataType>::iterator iType=sc->dataTypes_->begin(); 
      iType != sc->dataTypes_->end(); iType++) {
    writeHtmlDataTypeFiles(dir, *iType);
  }
}

/**.......................................................................
 * Write all files for a single data type
 */
static void writeHtmlDataTypeFiles(std::string& dir, ScriptDataType& type)
{
  writeHtmlDataTypeIndexFile(dir, type);
  writeHtmlDataTypeSynopsisFile(dir, type);
  writeHtmlDataTypeUsageFile(dir, type);
}

/**.......................................................................
 * Write the index file for a data type
 */
static void writeHtmlDataTypeIndexFile(std::string& dir, ScriptDataType& type)
{
  std::ostringstream os;
  os << dir << "/dataTypes/" << type.name_ << "_Index.html";
  std::ofstream fout(os.str().c_str(), ios::out);

  fout << "<html>" << std::endl;
  fout << "<head>" << std::endl;
  fout << "<title>" << type.name_ << " index file</title>" << std::endl;
  fout << "</head>" << std::endl << std::endl;

  fout << "<frameset rows=\"30%, 50%\" border=\"2\" frameborder=\"yes\""
       << "framespacing=\"0\">" << std::endl;
  fout << "  <frame name=\"botRightTop\"  src=\"" << type.name_ << "_Synopsis.html\">\n";
  fout << "  <frame name=\"botRightBot\"  src=\"" << type.name_ << ".html\">\n";
  fout << "</frameset>" << std::endl;

  writeHtmlFooter(fout);

  fout << "</html>";

  fout.close();
}

/**.......................................................................
 * Write the synopsis of a data type
 */
static void writeHtmlDataTypeSynopsisFile(std::string& dir, ScriptDataType& type)
{
  std::ostringstream os;
  os << dir << "/dataTypes/" << type.name_ << "_Synopsis.html";
  std::ofstream fout(os.str().c_str(), ios::out);

  writeHtmlHeader(fout, "../");
  
  fout << "<h2>The <span class=dataType>" << type.name_ 
  << "</span> data type (synopsis)" << "</h2>\n<hr>" << std::endl;

  writeDataType(fout, type);
  writeHtmlFooter(fout);

  fout << "</body>";
  fout << "</html>";

  fout.close();
}

static void writeHtmlDataTypeUsageFile(std::string& dir, ScriptDataType& type)
{
  std::ostringstream os ;
  os << dir << "/dataTypes/" << type.name_ << ".html";

  // Check if the file already exists -- if it does, don't create it!

  std::ifstream fin;
  fin.open(os.str().c_str(), ios::in);

  if(fin) {
    fin.close();
    return;
  }

  // Else just create a stub

  std::ofstream fout(os.str().c_str(), ios::out);

  writeHtmlHeader(fout, "../");

  fout << "<h2>The <span class=dataType>" << type.name_ 
  << "</span> data type (usage)" << "</h2>\n<hr>" << std::endl;

  fout << std::endl << std::endl;

  fout << "<h2><i>Context:</i></h2><p>\n\n";
  fout << "<h2><i>Examples:</i></h2><p>\n\n";

  fout << std::endl << std::endl;

  fout << "</body>";
  fout << "</html>";
  fout.close();
}

static void writeDataType(std::ofstream& fout, ScriptDataType& type)
{
  switch(type.id_) {
  case DT_UNK:
      fout << "<dl><dt>An experiment-specific datatype.<br><br>" << std::endl;
    break;
  case DT_SEXAGESIMAL:
      fout << "<dl><dt>A sexagesimal datatype.<br><br>" << std::endl;
    break;
  case DT_STRING:
      fout << "<dl><dt>A string datatype.<br><br>" << std::endl;
    break;
  case DT_UINT:
      fout << "<dl><dt>An unsigned integer datatype.<br><br>" << std::endl;
    break;
  case DT_INT:
      fout << "<dl><dt>An integer datatype.<br><br>" << std::endl;
    break;
  case DT_DOUBLE:
      fout << "<dl><dt>A double datatype.<br><br>" << std::endl;
    break;
  case DT_BOOL:
    {
      ChoiceType* context = (ChoiceType*)type.context_;

      fout << "<dl><dt>A boolean datatype.<br><br>Values are:</dt>" << std::endl;
      fout << "<dd><span class=\"enum\">true</span></dd>" << std::endl;
      fout << "<dd><span class=\"enum\">false</span></dd>" << std::endl;
    }
    break;
  case DT_CHOICE:
    {
      ChoiceType* context = (ChoiceType*)type.context_;

      fout << "A choice datatype.<br><br>Choices are:\n<dl><dl>" << std::endl;
      for(unsigned i=0; i < context->nchoice; i++) {
	fout << "  <dd><span class=\"enum\">" << context->choices[i].name 
	     << "</span>" << std::endl;
	fout << "<dl><dd>" << context->choices[i].explanation 
	     << "</dd></dl></dd>" << std::endl;
      }
      fout << "</dl></dl>" << std::endl;
    }
    break;
  case DT_SET:
    {
      SetType* context = (SetType*)type.context_;

      fout << "A set datatype.<br><br>Members are:\n<dl><dl>" << std::endl;
      for(unsigned i=0; i < context->nmember; i++) {
	fout << "  <dd><span class=\"enum\">" << context->members[i].name 
	     << "</span>" << std::endl;
	fout << "<dl><dd>" << context->members[i].explanation 
	     << "</dd></dl></dd>" << std::endl;
      }
      fout << "</dl></dl>" << std::endl;
    }
    break;
  default:
    break;
  }
}
