#include <stdlib.h>
#include <stdio.h>

#include "regdata.h"

#include "gcp/util/common/ArrayDataFrameManager.h"

using namespace gcp::control;

/*.......................................................................
 * Create a new array in which to store the values of uncalibrated
 * registers.
 *
 * Input:
 *  regmap       RegMap *  The register map.
 * Output:
 *  return   RegRawData *  The new container, or NULL on error.
 */
RegRawData *new_RegRawData(ArrayMap *arraymap, bool archivedOnly)
{
  RegRawData* raw=0;   /* The object to be returned */
  
  // Check arguments.

  if(!arraymap) {
    fprintf(stderr, "new_RegRawData: NULL regmap argument.\n");
    return NULL;
  };
  
  // Allocate the container of the object.

  raw = (RegRawData *) malloc(sizeof(RegRawData));
  if(!raw) {
    fprintf(stderr, "new_RegRawData: Insufficient memory for container.\n");
    return NULL;
  };
  

  // Before attempting any operation that might fail, initialize the
  // container at least up to the point at which it can safely be
  // passed to del_RegRawData().

  raw->nslot = arraymap->narchive;
  raw->slots = NULL;
  raw->fm = 0;  
  raw->pm = 0;  

  // Allocate and initialize the array of register slots.

  raw->slots = (int *) malloc(sizeof(long) * raw->nslot);
  if(!raw->slots) {
    fprintf(stderr, "new_RegRawData: Insufficient memory for monitor array.\n");
    return del_RegRawData(raw);
  };

  for(unsigned i=0; i<raw->nslot; i++)
    raw->slots[i] = 0L;

  // Allocate the data frame manager.  This can manage a frame which
  // contains either all registers (archivedOnly==false), or only
  // archived registers (archivedOnly==true).

  raw->fm = new gcp::util::ArrayDataFrameManager(archivedOnly, arraymap);
  if(!raw->fm) {
    fprintf(stderr, "new_RegRawData: Insufficient memory for frame manager.\n");
    return del_RegRawData(raw);
  };

  raw->pm = new gcp::util::PagerMonitor(raw->fm);
  if(!raw->pm) {
    fprintf(stderr, "new_RegRawData: Insufficient memory for pager monitor.\n");
    return del_RegRawData(raw);
  };

  return raw;
}

/*.......................................................................
 * Delete a monitor array object.
 *
 * Input:
 *  raw      RegRawData *  The object to be deleted.
 * Output:
 *  return   RegRawData *  Always NULL.
 */
RegRawData *del_RegRawData(RegRawData *raw)
{
  if(raw) {

    if(raw->slots) {
      free(raw->slots);
      raw->slots = 0;
    }

    if(raw->fm) {
      delete raw->fm;
      raw->fm = 0;
    }

    if(raw->pm) {
      delete raw->pm;
      raw->pm = 0;
    }

    free(raw);
    raw = 0;
  }

  return NULL;
}

/*.......................................................................
 * Return the number of bytes needed to record a raw data frame in
 * a network buffer.
 *
 * Input:
 *  raw     RegRawData *   The frame to characterize.
 * Output:
 *  return        long     The number of bytes required.
 */
long net_RegRawData_size(RegRawData *raw)
{
  return (long)raw->fm->sizeInBytesOfData();
}

/*.......................................................................
 * Pack the registers of a raw data frame into a network buffer.
 *
 * Input:
 *  net          NetBuf *   The network buffer in which to pack the
 *                          frame.
 *  raw      RegRawData *   The frame to be packed.
 * Output:
 *  return          int     0 - OK.
 *                          1 - Error.
 */
int net_put_RegRawData(NetBuf *net, RegRawData *raw)
{
  int status = net_put_char(net, raw->fm->sizeInBytesOfData(), 
			    raw->fm->frame()->getUcharPtr(raw->fm->byteOffsetInFrameOfData()));

  return status;
}

/*.......................................................................
 * Unpack the registers of a raw data frame from a network buffer.
 *
 * Input:
 *  net          NetBuf *   The network buffer from which to unpack the
 *                          frame.
 *  raw      RegRawData *   The frame in which to record the registers.
 * Output:
 *  return          int     0 - OK.
 *                          1 - Error.
 */
int net_get_RegRawData(NetBuf *net, RegRawData *raw)
{
  int status = net_get_char(net, raw->fm->sizeInBytesOfData(), 
			    raw->fm->frame()->getUcharPtr(raw->fm->byteOffsetInFrameOfData()));

  return status;
}
