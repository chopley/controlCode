#ifndef regtemplate_h
#define regtemplate_h

/*   ***** PLEASE READ BEFORE MAKING MODIFICATIONS *****
 * If you make ANY modifications to the register template
 * which make it incompatible with older archived register
 * map templates, you must increment the REGMAP_REVISION
 * macro at the top of regmap.h.
 */
#include <string>

/*
 * This file defines the templates required by new_RegMap() for
 * creating register maps. The resulting register map, and
 * accompanying objects are defined in regmap.h.
 */
#include "regmap.h"

#define ARRAY_DIM(array) (sizeof(array)/sizeof((array)[0]))

/*
 * Each board can have up to NBASE base addresses. The choice of
 * NBASE is arbitrary. Note, however, that if you change it, you
 * must also change REGMAP_REVISION in regmap.h.
 */
typedef enum {
  REG_BASE0,  /* To select the base address in RegBoardTemp::bases[0] */
  REG_BASE1,  /* To select the base address in RegBoardTemp::bases[1] */
  REG_BASE2,  /* To select the base address in RegBoardTemp::bases[2] */
  REG_BASE3,  /* To select the base address in RegBoardTemp::bases[3] */
  NBASE       /* The dimension of RegBoardTemp::bases[] */
} RegBase;

/*
 * The VME registers of a given board are specified via an array
 * of the following type. This is a template for creation of a
 * RegBlock object.
 */
struct RegBlockTemp {
  char name_[REG_NAME_LEN+1]; // The name of the block of registers 
  char axesHelp_[REG_NAME_LEN+1];
  unsigned flags_;            // A bit-set of RegFlags enumerators 
  RegAddrMode addr_mode_;     // The addressing mode of the register 
  RegBase base_;              // Use the base-address in board->bases[base] 
  unsigned address_;          // The address of the block in the address 
                              // space given by addr_mode (0 for REG_LOCAL) 
  gcp::util::CoordAxes* axes_;  // An axis specifier for this register
  std::string* comment_;

  //------------------------------------------------------------
  // Methods associated with this struct
  //------------------------------------------------------------

  // Constructors

  RegBlockTemp();
  RegBlockTemp(std::string comment, std::string name, std::string axesHelp,
	       unsigned flags,  unsigned address=0, 
	       unsigned nel0=1, unsigned nel1=0, unsigned nel2=0);

  // Destructor

  ~RegBlockTemp();

  // Copy constructors

  RegBlockTemp(const RegBlockTemp& temp);
  RegBlockTemp(RegBlockTemp& temp);

  // Assignment operators

  void operator=(const RegBlockTemp& temp);
  void operator=(RegBlockTemp& temp);

  void initialize();

  // Return the total number of register elements

  unsigned nEl();

  // Return the number of bytes per element of this register

  unsigned nBytePerEl();

  // Return the size in bytes of this register

  unsigned sizeInBytes();
};

/*
 * Each board of registers is described by an element of the following
 * type.
 */
typedef struct {
  char name_[REG_NAME_LEN+1]; // An unambiguous name for the board 
  RegBlockTemp* blocks_;      // The registers of the board 
  int nblock_;                // The number of register blocks in blocks[] 
  unsigned bases_[NBASE];     // An array of up to NBASE base addresses 
  char comment_[100];
} RegBoardTemp;

/**.......................................................................
 * Collect the register map template information into a single object.
 */
typedef struct {
  RegBoardTemp *boards;  // The array of register-board maps 
  unsigned nboard;       // The number of elements in boards[] 
  char comment_[100];
} RegTemplate;

RegMap *net_get_RegMap(gcp::control::NetBuf *net);

/* NB. del_RegMap() is prototyped in regmap.h */

int net_put_RegTemplate(RegTemplate *regtmp, gcp::control::NetBuf *net);
RegTemplate *net_get_RegTemplate(gcp::control::NetBuf *net);
int net_RegTemplate_size(RegTemplate *regtmp);

/*
 * The following destructor should only be applied the dynamically
 * allocated templates that are returned by net_get_RegTemplate().
 */
RegTemplate *del_RegTemplate(RegTemplate *regtmp);

#endif
