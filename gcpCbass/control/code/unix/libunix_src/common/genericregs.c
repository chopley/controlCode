
#include "arraymap.h"
#include "arraytemplate.h"
#include "miscregs.h"

#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/TimeVal.h"
#include "gcp/util/common/Directives.h"
#include "gcp/util/common/Sort.h"

#include <map>

using namespace gcp::util;
using namespace gcp::control;

// These must be defined by specific experiments

RegTemplate* specificAntennaTemplate();
RegTemplate* specificArrayRegMapTemplate();
ArrayTemplate* specificArrayTemplate();

/**.......................................................................
 * Create the  antenna register map.
 *
 * Output:
 *  return    RegMap *   The  register container object.
 */
RegMap *new_AntRegMap(void)
{
  RegMap* regmap = new RegMap(specificAntennaTemplate());
  return regmap;
}

/**.......................................................................
 * Pack the current register map for transmission over a network.
 *
 * Input:
 *  net   NetBuf *  The network buffer in which to pack the register
 *                  map. It is left to the caller to call
 *                  net_start_put() and net_end_put().
 * Output:
 *  return   int    0 - OK.
 *                  1 - Error.
 */
int net_put_AntRegMap(NetBuf *net)
{
  // Pack the register map via its template.
  
  return net_put_RegTemplate(specificAntennaTemplate(), net);
}

/**.......................................................................
 * Return the number of bytes needed by net_put_AntRegMap() to pack the
 * current register map into a network buffer.
 *
 * Output:
 *  return  long   The number of bytes required.
 */
long net_AntRegMap_size(void)
{
  return net_RegTemplate_size(specificAntennaTemplate());
}

/**.......................................................................
 * Create the  antenna register map.
 *
 * Output:
 *  return    RegMap *   The  register container object.
 */
RegMap *new_ArrayRegMap(void)
{
  RegMap* regmap = new RegMap(specificArrayRegMapTemplate());
  return regmap;
}

/**.......................................................................
 * Pack the current register map for transmission over a network.
 *
 * Input:
 *  net   NetBuf *  The network buffer in which to pack the register
 *                  map. It is left to the caller to call
 *                  net_start_put() and net_end_put().
 * Output:
 *  return   int    0 - OK.
 *                  1 - Error.
 */
int net_put_ArrayRegMap(NetBuf *net)
{
  // Pack the register map via its template.
  
  return net_put_RegTemplate(specificArrayRegMapTemplate(), net);
}

/**.......................................................................
 * Return the number of bytes needed by net_put_AntRegMap() to pack the
 * current register map into a network buffer.
 *
 * Output:
 *  return  long   The number of bytes required.
 */
long net_ArrayRegMap_size(void)
{
  return net_RegTemplate_size(specificArrayRegMapTemplate());
}

/**.......................................................................
 * Create the  array map.
 *
 * Output:
 *  return    ArrayMap *   The  array container object.
 */
ArrayMap *new_ArrayMap(void)
{
  return new ArrayMap(specificArrayTemplate());
}

/**.......................................................................
 * Pack the current array map for transmission over a network.
 *
 * Input:
 *  net   NetBuf *  The network buffer in which to pack the register
 *                  map. It is left to the caller to call
 *                  net_start_put() and net_end_put().
 * Output:
 *  return   int    0 - OK.
 *                  1 - Error.
 */
int net_put_ArrayMap(NetBuf *net)
{
  // Pack the arary map via its template.
  
  return net_put_ArrayTemplate(specificArrayTemplate(), net);
}

/**.......................................................................
 * Return the number of bytes needed by net_put_ArrayMap() to pack
 * the current register map into a network buffer.
 *
 * Output:
 *  return  long   The number of bytes required.
 */
long net_ArrayMap_size(void)
{
  return net_ArrayTemplate_size(specificArrayTemplate());
}

//-----------------------------------------------------------------------
// The following are for generating HTML documentation of the register
// map
//-----------------------------------------------------------------------

static void catDataType(std::ostringstream& os, RegMapBlock* block)
{
  os << " (";
  if(block->isBool())
    os << "boolean";
  else if(block->isUchar())
    os << "unsigned char";
  else if(block->isChar())
    os << "char";
  else if(block->isUshort())
    os << "unsigned short";
  else if(block->isShort())
    os << "short";
  else if(block->isUint())
    os << "unsigned integer";
  else if(block->isInt())
    os << "integer";
  else if(block->isFloat()) {
    if(block->isComplex())
      os << "complex ";
    os << "float";
  } 
  else if(block->isDouble())
    os << "double";
  else if(block->isUtc())
    os << "date";
  else
    os << "unknown";
  os << ")";
}

static void catArchived(std::ostringstream& os, RegMapBlock* block)
{
  os << " (";
  if(block->isArchived())
    os << "archived";
  else
    os << "not archived";
  os << ")";
}

static void catIntegrationType(std::ostringstream& os, RegMapBlock* block)
{
  os << " (";
  if(block->isSummed())
    os << "summed";
  else if(block->isUnioned())
    os << "unioned";
  else if(block->isPreAveraged())
    os << "pre-averaged";
  else if(block->isPostAveraged())
    os << "post-averaged";  
  else 
    os << "not integrated";
  os << ")";
}

static std::string typeStringOf(RegMapBlock* block)
{
  std::ostringstream os;

  catDataType(os, block);
  catArchived(os, block);
  catIntegrationType(os, block);

  return os.str();
}

/**.......................................................................
 * Generate an html listing of the array map
 */
void documentArrayMap()
{
  std::vector<ArrRegMap*> arrRegMaps;
  std::map<unsigned, ArrRegMap*> mapInds;

  // A map of array reg maps indexed by name

  std::map<std::string, ArrRegMap*> mapArrRegMap;
  std::vector<std::string> regMapNames;

  ArrayMap* arrayMap = 0;
  arrayMap = new_ArrayMap();

  // Find distinct register maps

  for(unsigned iRegMap=0; iRegMap < (unsigned)arrayMap->nregmap; iRegMap++) {

    ArrRegMap* arrRegMapPtr = arrayMap->regmaps[iRegMap];

    unsigned iFound;
    for(iFound=0; iFound < arrRegMaps.size(); iFound++)
      if(equiv_RegMap(arrRegMapPtr->regmap, arrRegMaps[iFound]->regmap)) {
	mapInds[iRegMap] = arrRegMaps[iFound];
	break;
      }
    
    // If no match was found, add the new register map to the list
    
    if(iFound == arrRegMaps.size()) {
      arrRegMaps.push_back(arrRegMapPtr);
      mapInds[iRegMap] = arrRegMapPtr;
      mapArrRegMap[arrRegMapPtr->name] = arrRegMapPtr;
      regMapNames.push_back(arrRegMapPtr->name);
    }
  }

  // Sort the register map names

  regMapNames = Sort::sort(regMapNames);

  //------------------------------------------------------------
  // Print header
  //------------------------------------------------------------

  std::cout << "<head>" << std::endl;
  std::cout << "<title>The  Register List.</title>" << std::endl;
  std::cout << "<link rel=stylesheet href=\"./StyleSheet.css\" type=\"text/css\">" << std::endl;
  std::cout << "</head>" << std::endl << std::endl;

  std::cout << "<body bgcolor=\"#add8e6\">" << std::endl;
  std::cout << "<font face=\"Verdana, Arial, Helvetica, sans-serif\""
	    << "size=\"2\" color=\"#000000\">" << std::endl;

  std::cout << "<center><a class=plain href=index.html>Index</a></center>" << std::endl;
  std::cout << "<h1>The  Register List.</h1>" << std::endl;
  std::cout << "A single frame of data is divided into a three-tiered hierarchy of "
	    << "related data items which I refer to as \"registers\" in this document. "
	    << "At the lowest level, a register consists of a block of data of arbitrary type, "
	    << "organized into a multi-dimensional array.  These blocks are grouped into "
	    << "logically-related units called \"boards\", "
	    << "typically representing data from a discrete hardware unit (a CAN module, for example) or "
	    << "a logically distinct operation (for example tracking, as opposed to receiver control). "
	    << "These boards are further grouped into \"register maps\", which represent data from related subsystems "
	    << "(an antenna, say, or the correlator)." << std::endl;

  std::cout << "<p>The <a class=plain href=#reglist>list of register maps</a> is given below.  Clicking on any one will take you to the list of boards for "
	    << "that register map (if several register maps are identical, only the first will be listed).  "
	    << "Clicking on a board will take you to the list of register blocks for that board, "
	    << "each listed using the following columns:" << std::endl;
  std::cout << "<p><pre>    registerMap.board.block[valid indices]       (data type) (archive status) (integration status)</pre>" << std::endl;

  //------------------------------------------------------------
  // List data types
  //------------------------------------------------------------

  std::cout << "<p>The data type is one of the following:" << std::endl
	    << "<ul>" << std::endl
	    << "<li><font color=\"#000080\">boolean</font> - A single-byte type, consisting of a 0 (false) or 1 (true), used for storing truth values</li>" << std::endl
	    << "<li><font color=\"#000080\">unsigned char</font> - A single-byte type, used for storing ASCII characters, or small unsigned integers (0 - 255)</li>" << std::endl
	    << "<li><font color=\"#000080\">char</font> - A single-byte type, used for storing ASCII characters, or small signed integers (0 - &#177 127)</li>" << std::endl
	    << "<li><font color=\"#000080\">unsigned short</font> - A two-byte type, used for storing integers (0 - 65535)</li>" << std::endl
	    << "<li><font color=\"#000080\">short</font> - A two-byte type, used for storing signed integers (0 - &#177 32767)</li>" << std::endl
	    << "<li><font color=\"#000080\">unsigned integer</font> - A four-byte type, used for storing unsigned integers (0 - 4.29497e+09)</li>" << std::endl
	    << "<li><font color=\"#000080\">integer</font> - A four-byte type, used for storing signed integers (0 - &#177 2.14748e+09)</li>" << std::endl
	    << "<li><font color=\"#000080\">float</font> - A four-byte type, used for storing floating-point values</li>" << std::endl
	    << "<li><font color=\"#000080\">double</font> - An 8-byte type, used for storing floating-point values</li>" << std::endl
	    << "<li><font color=\"#000080\">date</font> - An 8-byte type, used for storing high-precision dates.  " << std::endl
	    << "Registers of this type can be displayed by appending the following extensions to the register name: </li>" << std::endl
	    << "<ul>" << std::endl
	    << "<li><font color=\"#000080\">(none)</font> - a fractional MJD</li>" << std::endl
	    << "<li><font color=\"#000080\">.date</font> - a UTC date string</li>" << std::endl
	    << "<li><font color=\"#000080\">.time</font> - a time string</li>" << std::endl
	    << "</ul>" << std::endl
	    << "<li><font color=\"#000080\">complex float</font> - An 8-byte type, used for storing complex (re, im) floating-point values  " << std::endl
	    << "Registers of this type can be displayed by appending the following extensions to the register name: </li>" << std::endl
	    << "<ul>" << std::endl
	    << "<li><font color=\"#000080\">(none)</font> - a complex number</li>" << std::endl
	    << "<li><font color=\"#000080\">.real</font> - the real part only</li>" << std::endl
	    << "<li><font color=\"#000080\">.imag</font> - the imaginary part only</li>" << std::endl
	    << "<li><font color=\"#000080\">.amp</font> - the amplitude (magnitude) of the complex number</li>" << std::endl
	    << "<li><font color=\"#000080\">.phase</font> - the phase (in degrees) of the complex number</li>" << std::endl
	    << "</ul>" << std::endl
	    << "</ul>" << std::endl;

  //------------------------------------------------------------
  // List archive status
  //------------------------------------------------------------

  std::cout << "<p>The archive status indicates which registers are archived:" << std::endl
	    << "<ul>" << std::endl
	    << "<li><font color=\"#000080\">archived</font> - The register is archived</li>" << std::endl
	    << "<li><font color=\"#000080\">not archived</font> - The register is not archived</li>" << std::endl
	    << "</ul>" << std::endl;

  //------------------------------------------------------------
  // List integration status
  //------------------------------------------------------------

  std::cout << "<p>The integration status indicates how the register is treated when frames are combined before being written to disk:" << std::endl
	    << "<ul>" << std::endl
	    << "<li><font color=\"#000080\">summed</font> - The register is simply summed on integration</li>" << std::endl
	    << "<li><font color=\"#000080\">unioned</font> - The register is bit-wise unioned on integration</li>" << std::endl
	    << "<li><font color=\"#000080\">pre-averaged</font> - The register is averaged before being written to disk</li>" << std::endl
	    << "<li><font color=\"#000080\">post-averaged</font> - The register is summed before being written to disk, " << std::endl
	    << "and divided by the number of frames which were combined on read-in</li>" << std::endl
	    << "<li><font color=\"#000080\">not integrated</font> - Only the last value of the register is archived" << std::endl
	    << "</ul>" << std::endl;

  std::cout << "<hr>" << std::endl;

  //------------------------------------------------------------
  // List register maps
  //------------------------------------------------------------

  std::cout << "<a name=reglist></a>" << std::endl;
  std::cout << "<h1>Index of register maps</h1>" << std::endl;
  std::cout << "<dl>" << std::endl;

  for(unsigned iRegMap=0; iRegMap < (unsigned)arrayMap->nregmap; iRegMap++) {

    // Get the register map corresponding to this index, in sorted order

    ArrRegMap* arrRegMap = mapArrRegMap[regMapNames[iRegMap]];

    std::cout << "<dt><a class=plain href=#" << arrRegMap->name << ">" << arrRegMap->name << "</a></dt>" << std::endl
	      << "<dd>" << arrRegMap->comment_->c_str() << "</dd>" << std::endl;
  }
  std::cout << "</dl>" << std::endl;
  std::cout << "<br><br>Array map comprises a total of: " << arrayMap->nByte(false) << " bytes, "
	    << "of which " << arrayMap->nByte(true) << " are archived.<br><br>" << std::endl;

  //------------------------------------------------------------
  // Now iterate over distinct register maps, printing a listing 
  // of each one
  //------------------------------------------------------------

  for(unsigned iRegMap=0; iRegMap < arrRegMaps.size(); iRegMap++) {

    ArrRegMap* arrRegMap = mapArrRegMap[regMapNames[iRegMap]];
    RegMap*    regMap    = arrRegMap->regmap;
    
    // Iterate over boards of this register map

    std::vector<std::string> boardNames;
    std::map<std::string, RegMapBoard*> mapBoard;

    for(unsigned iBoard=0; iBoard < (unsigned)regMap->nboard_; iBoard++) {
      RegMapBoard* board = regMap->boards_[iBoard];
      mapBoard[board->name] = board;
      boardNames.push_back(board->name);
    }
    
    // Now sort the list of board names

    boardNames = Sort::sort(boardNames);

    //------------------------------------------------------------
    // List the boards of this register map
    //------------------------------------------------------------
    
    std::cout << std::endl << "<hr>" << std::endl;
    std::cout << "<a name=" << arrRegMap->name << "></a>" << std::endl;
    std::cout <<"<h2>" << arrRegMap->name << std::endl;
    std::cout <<"<h2>" << arrRegMap->comment_->c_str() << "</h2>" << std::endl;
    std::cout << "<dl>" << std::endl;

    for(unsigned iBoard=0; iBoard < (unsigned)regMap->nboard_; iBoard++) {
      RegMapBoard* board = mapBoard[boardNames[iBoard]];

      std::cout << "<dt><a class=plain href=#" << arrRegMap->name << "." << board->name << ">" << board->name << "</a></dt>" << std::endl
		<< "<dd>" << board->comment_->c_str() << "</dd>" << std::endl;
    }

    std::cout << "</dl>" << std::endl;
    std::cout << "<br><br>This register map comprises a total of: " << regMap->nByte(false) << " bytes, " << std::endl;
    std::cout << "of which " << regMap->nByte(true) << " are archived<br><br>" << std::endl;
    std::cout << "<a class=plain href=#reglist>Back to register map list</a>" << std::endl;

    //------------------------------------------------------------
    // Now iterate through the boards of this register map, listing 
    // the registers of each one
    //------------------------------------------------------------

    for(unsigned iBoard=0; iBoard < (unsigned)regMap->nboard_; iBoard++) {
      RegMapBoard* board = mapBoard[boardNames[iBoard]];

      std::cout << std::endl << "<hr>" << std::endl;
      std::cout << "<a name=" << arrRegMap->name << "." << board->name << "></a>" << std::endl;
      std::cout <<"<h2>" << arrRegMap->name << "." << board->name << std::endl;
      std::cout <<"<h2>" << board->comment_->c_str() << "</h2>" << std::endl;
      
      std::cout << "<dl>" << std::endl;

      // Now iterate over all registers of this board

      std::vector<std::string> regNames;
      std::map<std::string, RegMapBlock*> mapReg;

      for(unsigned iBlock=0; iBlock < (unsigned)board->nblock; iBlock++) {
	RegMapBlock* block = board->blocks[iBlock];

	regNames.push_back(block->name_);
	mapReg[block->name_] = block;
      }

      regNames = Sort::sort(regNames);

      //-------------------------------------------------------------
      // List the register of this board
      //-------------------------------------------------------------

      for(unsigned iBlock=0; iBlock < (unsigned)board->nblock; iBlock++) {
	RegMapBlock* block = mapReg[regNames[iBlock]];

	std::cout << "<dt><font color=\"#000080\">" << arrRegMap->name << "." << board->name 
		  << "." << block->name_  << *block->axes_  << "</font>"
		  << "<font color=\"660066\">" << typeStringOf(block) << "</font></dt>" << std::endl
		  << "<dd>" << block->comment_->c_str() << "</dd>" << std::endl;

      }
      std::cout << "</dl>" << std::endl;
      std::cout << "<br><br>This board comprises a total of: " << board->nByte(false) << " bytes, " << std::endl;
      std::cout << "of which " << board->nByte(true) << " are archived<br><br>" << std::endl;
      std::cout << "<a class=plain href=#" << arrRegMap->name << ">Back to board list</a>" << std::endl;
    }
  }

  gcp::util::TimeVal timeVal;
  timeVal.setToCurrentTime();

  std::cout << "<hr><p>Last modified by Erik Leitch (" << timeVal << ")" << std::endl;
  std::cout << "</body>" << std::endl;

  if(arrayMap != 0)
    delete arrayMap;
}
