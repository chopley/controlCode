#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/AxisRange.h"
#include "gcp/util/common/DataFrameNormal.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Directives.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/FrameFlags.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/receiver/specific/XMLBoardDataFrameManager.h"

#include "gcp/control/code/unix/libunix_src/common/scanner.h"

#include <iomanip>

using namespace gcp::receiver;
using namespace std;
using namespace MuxReadout;

/**.......................................................................
 * Constructor with initialization from a DataFrame object.
 */
XMLBoardDataFrameManager::
XMLBoardDataFrameManager(std::string regmap, std::string board,
			 bool archivedOnly) :
  BoardDataFrameManager(regmap, board, archivedOnly)
{}
        
/**.......................................................................
 * Constructor from a XMLData object
 */
void XMLBoardDataFrameManager::
setTo(MuxReadout::MuxXMLFile *xml) {}

/**.......................................................................
 * Destructor.
 */
XMLBoardDataFrameManager::~XMLBoardDataFrameManager() {}

/**.......................................................................
 * Main method for parsing registers out of the XML and copying values
 * into the data frame
 */
void XMLBoardDataFrameManager::
getRegisters(MuxReadout::MuxXMLFile* xml, vector<std::string>& containerNames,
	     std::string addressVectorName, std::string valueVectorName,
	     XMLRegisterMap& registers,     unsigned int boardNum)
{
  vector<std::string>::iterator containerIter = containerNames.begin();
  unsigned int level = 0;
  while(containerIter != containerNames.end())
  {
    if (!xml->down_to_subcontainer(*containerIter)) {

      while(level--) {
        xml->up_to_parent_container();
      }

      return;
    }

    ++containerIter;
    ++level;
  }
  
  // Get values of named registers

  vector<XMLRegisterMap::NamedRegister>::iterator regIter;
  for(regIter = registers.namedRegisters_.begin(); 
      regIter != registers.namedRegisters_.end(); regIter++) {

    if(xml->has_key_of_type_and_size(regIter->name_, regIter->typeName_, 1)) {

      if(regIter->typeName_ == "int") {

        vector<int> valueVector = xml->get_int_vec(regIter->name_);
        registers.set(regIter->name_, valueVector[0], boardNum);

      } else if(regIter->typeName_ == "double") {

        vector<double> valueVector = xml->get_double_vec(regIter->name_);
        registers.set(regIter->name_, valueVector[0], boardNum);

      } else if(regIter->typeName_ == "bool") {

        vector<bool> valueVector = xml->get_bool_vec(regIter->name_);
        registers.set(regIter->name_, valueVector[0] ? 1 : 0, boardNum);

      } else if(regIter->typeName_ == "string") {

        std::string value = xml->get_string(regIter->name_);
        registers.set(regIter->name_, value, boardNum);

      } else {
        ReportError("Unsupported data type - " << regIter->typeName_);
      }

    } else if(xml->has_key_of_type(regIter->name_, regIter->typeName_)) {

      if(regIter->typeName_ == "int") {

        vector<int> valueVector = xml->get_int_vec(regIter->name_);
        registers.set(regIter->name_, valueVector, boardNum);

      } else if(regIter->typeName_ == "double") {

        vector<double> valueVector = xml->get_double_vec(regIter->name_);
	registers.set(regIter->name_, valueVector, boardNum);

      } else {
        ReportError("Unsupported data type - " << regIter->typeName_);
      }
    }
  }

  // Get values of addressed registers
  
  vector<int> addressVector = xml->get_int_vec(addressVectorName);
  vector<int>::iterator addrIter = addressVector.begin();
  
  if(xml->has_key_of_type(valueVectorName, MuxReadout::TYPE_int)) {

    vector<int> valueVector = xml->get_int_vec(valueVectorName);
    vector<int>::iterator valueIter = valueVector.begin();
  
    while(addrIter != addressVector.end()) {
      registers.set(*addrIter, *valueIter, boardNum);
      ++addrIter;
      ++valueIter;
    }
  }
  
  if(xml->has_key_of_type(valueVectorName, MuxReadout::TYPE_double)) {
    vector<double> valueVector = xml->get_double_vec(valueVectorName);
    vector<double>::iterator valueIter = valueVector.begin();
  
    while(addrIter != addressVector.end()) {
      registers.set(*addrIter, *valueIter, boardNum);
      ++addrIter;
      ++valueIter;
    }
  }

  // Write the buffered values into the register frame

  registers.write();
  
  while(level--) {
    xml->up_to_parent_container();
  }

}

