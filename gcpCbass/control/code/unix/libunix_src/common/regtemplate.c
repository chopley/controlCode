#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regtemplate.h"
#include "lprintf.h"

#include "gcp/util/common/RegDate.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

using namespace std;
using namespace gcp::control;
using namespace gcp::util;

/*.......................................................................
 * Pack a register map template into a network buffer.
 *
 * Input:
 *  regtmp  RegTemplate *  The register map template to be packed.
 * Input/Output:
 *  net          NetBuf *  The network buffer in which to pack the
 *                         template. Note that it is the callers
 *                         responsibility to call net_start_put() and
 *                         net_end_put().
 * Output:
 *  return     int     0 - OK.
 *                     1 - Error.
 */
int net_put_RegTemplate(RegTemplate *regtmp, NetBuf *net)
{
  int board;   // The index of the board being packed 
  int block;   // The index of the block being packed 
  int i;
  
  // Check arguments.

  if(!regtmp || !net) {
    lprintf(stderr, "net_put_RegTemplate: NULL %s argument.\n",
	    !regtmp ? "regtmp" : "net");
    return 1;
  };

#ifdef USE_REGMAP_REV
  
  // Record the register-map object revision count.

  unsigned int regmap_revision = REGMAP_REVISION;
  if(net_put_int(net, 1, &regmap_revision))
    return 1;

#endif
  
  // Record the count of the number of boards.

  unsigned short nboard = regtmp->nboard;
  if(net_put_short(net, 1, &nboard))
    return 1;
  
  // Pack each board.

  for(board=0; board < (int)regtmp->nboard; board++) {
    RegBoardTemp *brd = regtmp->boards + board;
    unsigned short name_len = strlen(brd->name_);
    unsigned short nblock = brd->nblock_;
    
    // Record the name and size of the board.

    if(net_put_short(net, 1, &name_len) ||
       net_put_char(net, name_len, (unsigned char* )brd->name_) ||
       net_put_short(net, 1, &nblock))
      return 1;

    // Pack each block description.

    for(block=0; block < nblock; block++) {
      RegBlockTemp* blk = brd->blocks_ + block;
      
      // Assemble details of the block into appropriate datatypes for
      // packing.

      unsigned short name_len = strlen(blk->name_);
      unsigned int ltmp1[3];
      unsigned int ltmp2[4];

      ltmp1[0] = blk->flags_;
      ltmp1[1] = blk->addr_mode_;
      ltmp1[2] = blk->base_;

      ltmp2[0] = blk->address_;
      ltmp2[1] = blk->axes_->nEl(0);
      ltmp2[2] = blk->axes_->nEl(1);
      ltmp2[3] = blk->axes_->nEl(2);
      
      // Pack the details into the network buffer.

      if(net_put_short(net, 1, &name_len) ||
	 net_put_char(net, name_len, (unsigned char* )blk->name_) ||
	 net_put_int(net, 3, ltmp1) ||
	 net_put_int(net, 4, ltmp2))
	return 1;

    };
    
    // Pack the array of VME base addresses.

    {
      unsigned int bases_[NBASE];
      for(i=0; i<NBASE; i++)
	bases_[i] = brd->bases_[i];
      if(net_put_int(net, NBASE, bases_))
	return 1;
    };
  };
  return 0;
}

/*.......................................................................
 * Unpack a register map template from a network buffer.
 *
 * Input:
 *  net          NetBuf *  The network buffer from which to unpack the
 *                         template. Note that it is the callers
 *                         responsibility to call net_start_get() and
 *                         net_end_get().
 * Output:
 *  return  RegTemplate *  The unpacked register map template, or NULL
 *                         on error. This can be deleted via a call to
 *                         del_RegTemplate().
 */
RegTemplate *net_get_RegTemplate(NetBuf *net)
{
  RegTemplate *regtmp;   // The new template 
  unsigned board;        // The index of the board being processed 
  unsigned block;        // The index of the block being processed 
  int i;
  
  // Check arguments.

  if(!net) {
    lprintf(stderr, "net_get_RegTemplate: NULL net argument.\n");
    return NULL;
  };
  
  // Allocate the container of the template.

  regtmp = (RegTemplate *) malloc(sizeof(RegTemplate));
  if(!regtmp) {
    lprintf(stderr, "net_get_RegTemplate: Insufficient memory.\n");
    return NULL;
  };
  
  // Before attempting any operation that might fail, initialize the
  // container at least up to the point at which it can safely be
  // passed to del_RegTemplate().

  regtmp->boards = NULL;
  regtmp->nboard = 0;
  regtmp->comment_[0] = '\0';

#ifdef USE_REGMAP_REV
  
  // Unpack the register-map object revision count and see if the
  // register template can be decoded by this function.

  {
    unsigned int regmap_revision;
    if(net_get_int(net, 1, &regmap_revision))
      return del_RegTemplate(regtmp);
    if(regmap_revision != REGMAP_REVISION) {
      lprintf(stderr, "net_get_RegTemplate: Incompatible register map.\n");
      return del_RegTemplate(regtmp);
    };
  };
#endif
  
  // Unpack the count of the number of boards.

  {
    unsigned short nboard;
    if(net_get_short(net, 1, &nboard))
      return del_RegTemplate(regtmp);
    regtmp->nboard = nboard;
    if(nboard < 1) {
      lprintf(stderr, "net_get_RegTemplate: nboard <= 0.\n");
      return del_RegTemplate(regtmp);
    };
  };
/*
 * Allocate the array of boards.
 */
  regtmp->boards = (RegBoardTemp *) malloc(sizeof(RegBoardTemp)*regtmp->nboard);
  if(!regtmp->boards) {
    lprintf(stderr, "net_get_RegTemplate: Insufficient memory.\n");
    return del_RegTemplate(regtmp);
  };
/*
 * Initialize the array.
 */
  for(board=0; board<regtmp->nboard; board++) {
    RegBoardTemp *brd = regtmp->boards + board;
    brd->name_[0] = '\0';
    brd->blocks_ = NULL;
    brd->nblock_ = 0;
    brd->comment_[0] = '\0';
    for(i=0; i < NBASE; i++)
      brd->bases_[i] = 0;
  };
  
  // Un-pack each board.

  for(board=0; board<regtmp->nboard; board++) {
    RegBoardTemp* brd = regtmp->boards + board;
    unsigned short name_len;
    unsigned short nblock;
    
    // Un-pack the name and size of the board.

    if(net_get_short(net, 1, &name_len))
      return del_RegTemplate(regtmp);

    if(name_len > REG_NAME_LEN) {
      lprintf(stderr, "net_get_RegTemplate: Board name too int.\n");
      return del_RegTemplate(regtmp);
    };

    if(net_get_char(net, name_len, (unsigned char* )brd->name_) ||
       net_get_short(net, 1, &nblock))
      return del_RegTemplate(regtmp);

    brd->name_[name_len] = '\0';
    brd->nblock_ = nblock;

    if(brd->nblock_ < 1) {
      lprintf(stderr, "net_get_RegTemplate: nblock <= 0.\n");
      return del_RegTemplate(regtmp);
    };
    
    // Allocate the array of blocks.

    brd->blocks_ = (RegBlockTemp *) malloc(sizeof(RegBlockTemp) * brd->nblock_);
    if(!brd->blocks_) {
      lprintf(stderr, "net_get_RegTemplate: Insufficient memory.\n");
      return del_RegTemplate(regtmp);
    };
    
    // Initialize the array.

    for(block=0; block < (unsigned)brd->nblock_; block++) {
      RegBlockTemp* blk = brd->blocks_ + block;
      blk->name_[0] = '\0';
      blk->axesHelp_[0] = '\0';
      blk->flags_ = 0;
      blk->addr_mode_ = ADDR_DEFAULT;
      blk->base_ = REG_BASE0;
      blk->address_ = 0;
      blk->axes_ = 0;
      blk->comment_ = 0;
      blk->comment_ = new std::string("");
    };
    
    // Un-pack each block description into the newly allocate array.

    for(block=0; block < (unsigned)brd->nblock_; block++) {
      RegBlockTemp *blk = brd->blocks_ + block;
      
      // Assemble details of the block into appropriate datatypes for
      // packing.

      unsigned short name_len;
      unsigned int ltmp1[3];
      unsigned int ltmp2[4];
      
      // Un-pack the details of the latest block.

      if(net_get_short(net, 1, &name_len))
	return del_RegTemplate(regtmp);

      if(name_len > REG_NAME_LEN) {
	lprintf(stderr, "net_get_RegTemplate: Block name too int.\n");
	return del_RegTemplate(regtmp);
      };

      if(net_get_char(net, name_len, (unsigned char* )blk->name_) ||
	 net_get_int(net, 3, ltmp1) ||
	 net_get_int(net, 4, ltmp2))
	return del_RegTemplate(regtmp);
	
      // Record the details.

      blk->name_[name_len] = '\0';
      blk->flags_     = ltmp1[0];
      blk->addr_mode_ = (RegAddrMode)ltmp1[1];
      blk->base_      = (RegBase)ltmp1[2];
      blk->address_   = ltmp2[0];
      blk->axes_      = new CoordAxes(ltmp2[1], ltmp2[2], ltmp2[3]);

      if(blk->nEl() < 1) {
	lprintf(stderr, "net_get_RegTemplate: nreg <= 0.\n");
	return del_RegTemplate(regtmp);
      };

    };
    
    // Unpack the array of base addresses.

    {
      unsigned int bases[NBASE];
      if(net_get_int(net, NBASE, bases))
	return del_RegTemplate(regtmp);
      for(i=0; i < NBASE; i++)
	brd->bases_[i] = bases[i];
    };
  };

  return regtmp;
}

/*.......................................................................
 * Delete a register template returned previously by net_get_RegTemplate().
 *
 * Input:
 *  regtmp   RegTemplate *   The template to be deleted.
 * Output:
 *  return   RegTemplate *   The deleted template (ie. NULL).
 */
RegTemplate *del_RegTemplate(RegTemplate *regtmp)
{
  if(regtmp) {
    if(regtmp->boards) {
      for(unsigned i=0; i < regtmp->nboard; i++) {
	RegBoardTemp* board = regtmp->boards + i;
	if(board->blocks_) {

	  // We must properly destruct the CoordAxes pointer or the
	  // memory won't be freed

	  for(unsigned iblk=0; iblk < board->nblock_; iblk++) {
	    RegBlockTemp *block = board->blocks_ + iblk;

	    if(block->axes_ != 0)
	      delete block->axes_;

            if(block->comment_ != 0)
              delete block->comment_;
	  }

	  free(board->blocks_);
	}
      };
      free(regtmp->boards);
    };
    free(regtmp);
  };
  return NULL;
}

/*.......................................................................
 * Return the space needed to pack a given register-map template
 * into a NetBuf network buffer.
 *
 * Input:
 *  regtmp   RegTemplate *  The set to be characterized.
 * Output:
 *  return     int    The number of bytes needed to pack a full
 *                     set of register ranges.
 */
int net_RegTemplate_size(RegTemplate *regtmp)
{
  unsigned board;  // The index of the board being processed 
  int block;  // The index of the block being processed 
  int size = 0;   // The byte-count to be returned 

  // Reserve space for the count of the number of register boards.

  size += NET_SHORT_SIZE;
  
  // Reserve space for the details of each VME board.

  for(board=0; board<regtmp->nboard; board++) {
    RegBoardTemp *brd = regtmp->boards + board;
    size += NET_SHORT_SIZE +                  // name_len 
      NET_CHAR_SIZE * strlen(brd->name_) +    // name_[] 
      NET_SHORT_SIZE;                         // nblock 
    
    // Reserve space for the details of each VME register block on the
    // board.

    for(block=0; block < brd->nblock_; block++) {
      RegBlockTemp* blk = brd->blocks_ + block;
      size += NET_SHORT_SIZE +                 // name_len 
	NET_CHAR_SIZE * strlen(blk->name_) +   // name_[] 
	NET_INT_SIZE * 3 +                     // flags_, addr_mode_, base_ 
	NET_INT_SIZE * 4;                      // address, netl0, nel1, nel2
    };
    
    // Reserve space for the array of board addresses.

    size += NET_INT_SIZE * NBASE;
  };

  return size;
}

/*.......................................................................
 * Retrieve a register map template from a network buffer and convert
 * it to a register map.
 *
 * Input:
 *  net    NetBuf *  The network buffer from which to unpack the register
 *                   map. It is left to the caller to call
 *                   net_start_get() and net_end_get().
 * Output:
 *  return RegMap *  The register map, or NULL on error.
 */
RegMap *net_get_RegMap(NetBuf *net)
{
  RegTemplate *rt; // The template of the register map 
  RegMap *regmap;  // The corresponding register map 
  
  // Retrieve the register template from the network buffer.

  rt = net_get_RegTemplate(net);
  if(!rt)
    return NULL;
  
  // Construct the register map from the template.

  regmap = new RegMap(rt);
  
  // Discard the redundant template.

  rt = del_RegTemplate(rt);
  
  // Return the register map (this will be NULL if new_RegMap()
  // failed).

  return regmap;
}

//=======================================================================
// Methods of the RegBlockTemp struct
//=======================================================================

RegBlockTemp::RegBlockTemp()
{
  initialize();
}

/**.......................................................................
 * Initialize a RegBlockTemp object
 */
void RegBlockTemp::initialize()
{
  flags_     = 0x0;
  address_   = 0x0;

  addr_mode_ = ADDR_DEFAULT;

  // Set the base address to the default

  base_ = REG_BASE0;  

  axes_      = 0;
  comment_   = 0;
}

/**.......................................................................
 * Return the total size in bytes of a register
 */
RegBlockTemp::RegBlockTemp(std::string comment, std::string name, std::string axesHelp,
			   unsigned flags, unsigned address, 
			   unsigned nel0,  unsigned nel1, unsigned nel2)
{
  initialize();

  flags_   = flags;
  address_ = address;

  // Check that the string is not too long

  if(name.size() > REG_NAME_LEN) {
    ThrowError("Register name is too long: " << name);
  }

  // And copy the string

  strcpy(name_, name.c_str());
  
  // Check that the string is not too long

  if(axesHelp.size() > REG_NAME_LEN) {
    ThrowError("Axes help is too long: " << name);
  }

  // And copy the string

  strcpy(axesHelp_, axesHelp.c_str());

  // Store the comment

  comment_ = new std::string(comment);

  // Construct the axis specifier

  axes_ = new CoordAxes(nel0, nel1, nel2);
}

RegBlockTemp::RegBlockTemp(const RegBlockTemp& temp)
{
  initialize();
  *this = temp;
}

RegBlockTemp::RegBlockTemp(RegBlockTemp& temp)
{
  initialize();
  *this = temp;
}  

void RegBlockTemp::operator=(const RegBlockTemp& temp)
{
  *this = (RegBlockTemp&) temp;
}

void RegBlockTemp::operator=(RegBlockTemp& temp)
{
  if(axes_) {
    delete axes_;
  }

  axes_         = 0;

  if(comment_) {
    delete comment_;
  }

  comment_      = 0;

  // Now copy pertinent members

  strcpy(name_, temp.name_);
  strcpy(axesHelp_, temp.axesHelp_);
  
  flags_     = temp.flags_;
  addr_mode_ = temp.addr_mode_;
  base_      = temp.base_;

  axes_      = new CoordAxes(temp.axes_->nEl(0), temp.axes_->nEl(1), temp.axes_->nEl(2));
  comment_   = new std::string(*temp.comment_);
}

/**.......................................................................
 * Destructor
 */
RegBlockTemp::~RegBlockTemp()
{
  if(axes_ != 0) {
    delete axes_;
    axes_ = 0;
  }

  if(comment_ != 0) {
    delete comment_;
    comment_ = 0;
  }
}

/**.......................................................................
 * Return the total number of register elements
 */
unsigned RegBlockTemp::nEl()
{
  return axes_->nEl();
}

/**.......................................................................
 * Return the total size in bytes of a register
 */
unsigned RegBlockTemp::sizeInBytes()
{
  // Return the size of this register, in bytes.

  return axes_->nEl() * nBytePerEl();
}

/**.......................................................................
 * Return the size in bytes of a single element of a register
 */
unsigned RegBlockTemp::nBytePerEl()
{
  unsigned nBytePerReg = sizeof(unsigned int);

  // See if a type size specifier was given

  if((flags_ & REG_BOOL) || (flags_ & REG_CHAR) || (flags_ & REG_UCHAR))
    nBytePerReg = sizeof(unsigned char);

  else if((flags_ & REG_SHORT) || (flags_ & REG_USHORT))
    nBytePerReg = sizeof(unsigned short);

  else if((flags_ & REG_INT) || (flags_ & REG_UINT))
    nBytePerReg = sizeof(unsigned int);

  else if(flags_ & REG_FLOAT)
    nBytePerReg = sizeof(float);

  else if(flags_ & REG_DOUBLE)
    nBytePerReg = sizeof(double);

  else if(flags_ & REG_UTC)
    nBytePerReg = sizeof(gcp::util::RegDate::Data);

  // Check for complex registers

  if(flags_ & REG_COMPLEX)
    nBytePerReg *= 2;
  
  // Return the size of a single element of this register, in bytes.

  return nBytePerReg;
}
