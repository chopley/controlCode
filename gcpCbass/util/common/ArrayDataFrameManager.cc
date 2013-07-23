#define __FILEPATH__ "util/common/ArrayDataFrameManager.cc"

#include "gcp/util/common/ArrayDataFrameManager.h"
#include "gcp/util/common/DataFrameNormal.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/control/code/unix/libunix_src/common/scanner.h"

using namespace gcp::util;
using namespace std;

/**.......................................................................
 * Constructor with no resizing of the initially zero-length DataFrame
 * buffer.  This constructor doesn't intialize the antenna number
 * associated with this manager object.
 */
void ArrayDataFrameManager::initialize(ArrayMap* arrayMap, bool archivedOnly)
{
  unsigned frameSize;

  if(arrayMap == 0) {
    if((arrayMap_ = new_ArrayMap())==0)
      throw Error("ArrayDataFrameManager::ArrayDataFrameManager: "
		  " Unable to allocate arrmap_.\n");
  } else
    arrayMap_ = alias_ArrayMap(arrayMap);
  
  // If we are only recording archived registers, frame size is the
  // size of the archived register map

  frameSize = SCAN_BUFF_BYTE_SIZE(arrayMap_->nByte(archivedOnly));

  // Make the frame large enough to accomodate the register map

  frame_ = new gcp::util::DataFrameNormal(frameSize);

  // Initialize the nBuffer variable

  nBuffer_ = frameSize;
}

/**.......................................................................
 * This constructor doesn't intialize the antenna number
 * associated with this manager object.
 */
ArrayDataFrameManager::
ArrayDataFrameManager(bool archivedOnly, ArrayMap* arrayMap) :
  ArrayMapDataFrameManager(archivedOnly)
{
  initialize(arrayMap, archivedOnly);
}

/**.......................................................................
 * Copy constructor
 */
ArrayDataFrameManager::ArrayDataFrameManager(ArrayDataFrameManager& fm) :
  ArrayMapDataFrameManager(fm.archivedOnly_)
{
  initialize(fm.arrayMap_, fm.archivedOnly_);
  operator=(fm);
}

/**.......................................................................
 * Destructor.
 */
ArrayDataFrameManager::~ArrayDataFrameManager() 
{
  if(arrayMap_ != 0) {
    arrayMap_ = del_ArrayMap(arrayMap_);
    arrayMap_ = 0;
  }
}

/**.......................................................................
 * Assignment operators.
 */
void ArrayDataFrameManager::operator=(ArrayDataFrameManager& fm)
{
  ArrayMapDataFrameManager::operator=((ArrayMapDataFrameManager&) fm);
}

/**.......................................................................
 * Find the register map corresponding to an antenna data frame
 */
ArrRegMap* ArrayDataFrameManager::findAntennaRegMap(AntennaDataFrameManager& fm)
{
  return arrayMap_->findArrRegMap((char*)fm.getAnt().getAntennaName().c_str());
}

/**.......................................................................
 * Write an antenna data frame into our array frame
 */
void ArrayDataFrameManager::writeAntennaRegMap(AntennaDataFrameManager& fm,
					       bool lockFrame)
{
  ArrRegMap* aregmap = findAntennaRegMap(fm);

  if(aregmap==0) 
    ThrowError("No register map: " << fm.getAnt().getAntennaName() 
	       << " found in array map");

  // Call the base-class method to write the register map

  writeRegMap(aregmap, fm, lockFrame);
}

/**.......................................................................
 * Write a bolometer data frame into our array frame
 */
void ArrayDataFrameManager::writeGenericRegMap(RegMapDataFrameManager& fm,
					       bool lockFrame, string regMapName)
{
  ArrRegMap* aregmap = arrayMap_->findArrRegMap(regMapName);

  if(aregmap==0) 
    ThrowError("No register map: " << regMapName << "found in array map");

  // Call the base-class method to write the register map

  writeRegMap(aregmap, fm, lockFrame);
}

