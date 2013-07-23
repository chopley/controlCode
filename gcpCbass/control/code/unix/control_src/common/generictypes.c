#include <float.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/PointingMode.h"

#include "genericcontrol.h"
#include "genericscript.h"
#include "pathname.h"
#include "scanner.h" /* SCAN_xxx_HARDWARE_INTERVAL */
#include "navigator.h"
#include "generictypes.h"
#include "arcfile.h"
#include "genericscheduler.h"

#include "gcp/control/code/unix/libtransaction_src/TransactionManager.h"

#include "gcp/util/common/Directives.h"

#include "gcp/grabber/common/Flatfield.h"
#include "gcp/grabber/common/Channel.h"

#include "gcp/util/common/PointingTelescopes.h"

#include <sstream>

// Using namespace declarations.  Note that I cannot at present
// include gcp::util because of a namespace conflict with DataType,
// which is most satisfactorily resolved by fully qualifying all
// gcp::util types in this file.

using namespace gcp::control;
using namespace std;

static Enumerator OptCamEn[] = {
  {"grab",    OPTCAM_GRAB},
  {"id",      OPTCAM_ID},
  {"rbc",     OPTCAM_RBC},
  {"open",    OPTCAM_OPEN},
  {"close",   OPTCAM_CLOSE},
  {"preset",  OPTCAM_PRESET},
  {"stop",    OPTCAM_STOP},
  {"on",      OPTCAM_ON},
  {"off",     OPTCAM_OFF},
  {"inc",     OPTCAM_INC},
  {"dec",     OPTCAM_DEC},
  {"low",     OPTCAM_LOW},
  {"mid",     OPTCAM_MID},
  {"high",    OPTCAM_HIGH},
  {"s100",    OPTCAM_100},
  {"s250",    OPTCAM_250},
  {"s500",    OPTCAM_500},
  {"s1000",   OPTCAM_1000},
  {"s2000",   OPTCAM_2000},
  {"s4000",   OPTCAM_4000},
  {"s10000",  OPTCAM_10000},
  {"center",  OPTCAM_CENTER}
};
int OptCamNen = DIMENSION(OptCamEn);

/*-----------------------------------------------------------------------
 * The Date datatype uses astrom.h::input_utc() to read UTC date and
 * time values like:
 *
 *  dd-mmm-yyyy hh:mm:ss.s
 *
 * where mmm is a 3-letter month name. It stores the date as a Modified
 * Julian Date in DoubleVariable's. It provides + and - operators for
 * the addition and subtraction of Time values, along with all of the
 * relational operators except the ~ operator.
 */

/*
 * An object of following type is stored with the Date datatype symbol.
 */
typedef struct {
  ScOperator *add_op;  /* An operator proc that adds a time to a date */
  ScOperator *sub_op;  /* An operator proc that subtracts a time from a date */
  DataType *add_dt;  /* The Interval datatype used in addition */
} DateContext;

static DT_PARSE(parse_date);
static DT_CONST(const_date);
static DT_PARSE(parse_date);
static DT_CONST(const_date);
static DT_PRINT(print_date);
static OPER_FN(date_add_fn);
static OPER_FN(date_sub_fn);
static DT_ITER(date_iterate);

/*-----------------------------------------------------------------------
 * The Register datatype uses regmap.h::input_RegMapReg() to read
 * standard SZA archive-register specifications (eg. board.name[3-4]).
 *
 * It supports the == and ~ relational operators.
 */
static DT_CONST(const_reg);
static DT_PRINT(print_reg);
static DT_RELFN(equal_reg);
static DT_RELFN(in_reg);

/*-----------------------------------------------------------------------
 * The Wdir datatype is used to record writable directory names.
 * The path name is stored in a StringVariable. It supports
 * tilde expansion of home directories.
 */
static DT_CHECK(check_wdir);

/*-----------------------------------------------------------------------
 * The Dir datatype is used to record the names of accessible directories.
 *
 * The path name is stored in a StringVariable. It supports
 * tilde expansion of home directories.
 */
static DT_CHECK(check_dir);

/*-----------------------------------------------------------------------
 * The IntTime datatype is used to specify the hardware integration time
 * of the SZA electronics, as a power-of-2 exponent to be used to scale
 * the basic sample interval of 4.096e-4 seconds.
 */
static DT_CHECK(check_inttime);

/*-----------------------------------------------------------------------
 * The Board datatype is used to specify an archive register board by
 * name.
 */
static DT_CONST(const_board);
static DT_PRINT(print_board);

/*-----------------------------------------------------------------------
 * The Time datatype is used to specify a time of day (UTC or LST), or
 * a time interval.
 */
static DT_CHECK(check_time);
static DT_ITER(time_iterate);

/*-----------------------------------------------------------------------
 * The Interval datatype is used to specify a time interval.
 */
static DT_CONST(const_interval);
static DT_PRINT(print_interval);

/*-----------------------------------------------------------------------
 * The Source datatype is used for specification of a source by its name
 * in the source catalog.
 */
static DT_CONST(const_source);
static DT_PRINT(print_source);
static DT_RELFN(equal_source);

/*-----------------------------------------------------------------------
 * The Scan datatype is used for specification of a scan by its name
 * in the scan catalog.
 */
static DT_CONST(const_scan);
static DT_PRINT(print_scan);
static DT_RELFN(equal_scan);

/*-----------------------------------------------------------------------
 * The Latitude datatype is used to specify the latitude of a location
 * on the surface of the Earth.
 */
static DT_CHECK(check_latitude);

/*-----------------------------------------------------------------------
 * The Longitude datatype is used to specify the longitude of a location
 * on the surface of the Earth.
 */
static DT_CHECK(check_longitude);

/*-----------------------------------------------------------------------
 * The Azimuth datatype is used to specify a target azimuth for the
 * telescope. The azimuths of the compass points are N=0, E=90, S=180
 * and W=270 degrees.
 */
static DT_CHECK(check_azimuth);

/*-----------------------------------------------------------------------
 * The DeckAngle datatype is used to specify the position of the rotating
 * platform on which the dishes are mounted.
 */
static DT_CHECK(check_deckangle);

/*-----------------------------------------------------------------------
 * The Elevation datatype is used to specify the target elevation angle
 * of the telescope.
 */
static DT_CHECK(check_elevation);

/*-----------------------------------------------------------------------
 * The PointingOffset datatype is used to specify a temporary offset of
 * one of the telescope axes from the current pointing.
 */
static DT_CHECK(check_pointingoffset);

/*-----------------------------------------------------------------------
 * The Flexure datatype is used to specify the degree of gravitational
 * drooping of the telescope as a function of elevation.
 */
static DT_CHECK(check_flexure);

/*-----------------------------------------------------------------------
 * The Tilt datatype is used to specify the misalignment tilt of a
 * telescope axis.
 */
static DT_CHECK(check_tilt);

/*-----------------------------------------------------------------------
 * The Altitude datatype is used to specify the height of the telescope
 * above the standard geodetic spheroid.
 */
static DT_CHECK(check_altitude);

/*-----------------------------------------------------------------------
 * The GpibDev datatype is used to select a device on the GPIB bus.
 */
static DT_CHECK(check_gpib_dev);

/*-----------------------------------------------------------------------
 * The GpibCmd datatype is used to specify a command string to be sent
 * to a device on the GPIB bus.
 */
static DT_CHECK(check_gpib_cmd);

/*-----------------------------------------------------------------------
 * The Attenuation datatype is used to specify attenuation levels over
 * the range 0..31db.
 */
static DT_CHECK(check_attn_fn);

/*-----------------------------------------------------------------------
 * The OptCamCount datatype is used to specify a stepper motor count, or
 * on/off for the stepper motor.
 */
static DT_CONST(const_optcam);
static DT_PRINT(print_optcam);
static DT_RELFN(equal_optcam);
static DT_RELFN(gt_optcam);

/*-----------------------------------------------------------------------
 * The Schedule datatype is used for entering the name and arguments
 * of a schedule.
 */
static DT_CONST(const_script);
static DT_PRINT(print_script);
static int is_script_file(int c);
static DT_RELFN(equal_script);

/*-----------------------------------------------------------------------
 * The SlewRate datatype is used for specifying the percentage slew rate
 * of a telescope drive axis.
 */
static DT_CHECK(check_slew_rate);

/**-----------------------------------------------------------------------
 * Iterators for enumerated types
 */

static DT_ITER(ant_iterate);

/*-----------------------------------------------------------------------
 * The TransDev datatype is used for specification of a device by its name
 * in the transaction catalog.
 */
static DT_CONST(const_transdev);
static DT_PRINT(print_transdev);
static DT_RELFN(equal_transdev);


//-----------------------------------------------------------------------
// Date                                                                  
//-----------------------------------------------------------------------

/*.......................................................................
 * Define a date datatype and install it in the current scope.
 *
 * Input:
 *  sc          Script *   The script environment in which to install
 *                         the datatype.
 *  name          char *   The name to give to the datatype.
 * Output:
 *  return    DataType *   The new datatype.
 */
DataType *add_DateDataType(Script *sc, char *name)
{
  DateContext *context;    /* The type-specific context data */
  DataType *dt;            /* The object to be returned */
/*
 * Check arguments.
 */
  if(!sc || !name) {
    lprintf(stderr, "add_DateDataType: NULL argument(s).\n");
    return NULL;
  };
/*
 * Allocate an external context object for the datatype.
 */
  context = (DateContext* )new_ScriptObject(sc, NULL, sizeof(DateContext));
  if(!context)
    return NULL;
/*
 * Initialize the context data of the data-type.
 */
  context->add_op = NULL;
  context->sub_op = NULL;
  context->add_dt = find_DataType(sc, NULL, "Interval");
  if(!context->add_dt) {
    lprintf(stderr, "new_DataType(%s): Can't find the Interval datatype.\n",
	    name);
    return NULL;
  };
/*
 * Create the datatype.
 */
  dt = new_DataType(sc, name, DT_BUILTIN, context, sizeof(DoubleVariable),
		    0, parse_date, const_date, print_date, sc_equal_double,
		    sc_gt_double, 0, date_iterate, "Interval");
  if(!dt)
    return NULL;
/*
 * Create the addition and subtraction operators.
 */
  context->add_op = new_ScOperator(sc, dt, 2, date_add_fn);
  if(!context->add_op)
    return NULL;
  context->sub_op = new_ScOperator(sc, dt, 2, date_sub_fn);
  if(!context->sub_op)
    return NULL;
/*
 * Add the data-type to the symbol table.
 */
  if(!add_ScriptSymbol(sc, name, SYM_DATATYPE, dt))
    return NULL;
  return dt;
}

/*.......................................................................
 * Parse a date expression.
 */
static DT_PARSE(parse_date)
{
  DateContext *dc;      /* The date specific datatype members */
/*
 * Get the date-specific members of the data-type.
 */
  dc = (DateContext* )dt->context;
/*
 * Parse an expression of the form:
 *
 *   date_operand {+ time_operand} _clos_
 *
 * Eg. 15-apr-1995
 * or  15-apr-1995 + 12:34
 * or  15-apr-1995 + 12:00 + 0:5
 *
 * Start by reading the mandatory date operand.
 */
  if(parse_operand(sc, dt, 0, stream, e))
    return 1;
/*
 * Now read the optional addition and/or subtraction expression.
 */
  while(1) {
    int doadd;    /* True for '+', false for '-' */
/*
 * Find the start of the next term.
 */
    if(input_skip_space(stream, 1, 0))
      return 1;
/*
 * Check for and skip any following + or - operator.
 */
    switch(stream->nextc) {
    case '+':
      if(input_skip_white(stream, 1, 1))
	return 1;
      doadd = 1;
      break;
    case '-':
      if(input_skip_white(stream, 1, 1))
	return 1;
      doadd = 0;
      break;
    default:
      return 0;   /* End of expression */
      break;
    };
/*
 * Read the following Time operand.
 */
    if(parse_operand(sc, dc->add_dt, 0, stream, e))
      return 1;
/*
 * Push the specified addition/subtraction operator onto the
 * stack. This takes the current value of the date and the
 * new operand as its arguments.
 */
    if(!add_OpFnOper(sc, e, doadd ? dc->add_op : dc->sub_op))
      return 1;
  };
}

/*.......................................................................
 * Parse a date constant.
 */
static DT_CONST(const_date)
{
  Variable *var;        /* The new date-constant */
  double utc;           /* The UTC as a Modified Julian Date */
/*
 * Parse the date and time and convert it to a Modified Julian Date.
 */
  if(input_utc(stream, 1, 0, &utc))
    return 1;
/*
 * Record the date in a new variable and push the variable onto the
 * expression stack.
 */
  var = new_Variable(sc, dt->atom_reg);
  if(!var || !add_LoadOper(sc, e, var))
    return 1;
/*
 * Initialize the value of the constant.
 */
  DOUBLE_VARIABLE(var)->d = utc;
  return 0;
}

/*.......................................................................
 * Print a date value.
 */
static DT_PRINT(print_date)
{
  return output_utc(output, "", 0, 0, DOUBLE_VARIABLE(var)->d);
}

/*.......................................................................
 * Define the date addition operator function. This is a binary operator
 * that expects a date value on its left and a an interval value on its
 * right. The result is a new date.
 */
static OPER_FN(date_add_fn)
{
  ListNode *node;      /* A node of the argument list */
  Variable *v1, *v2;   /* The input arguments */
/*
 * Get the two arguments.
 */
  node = args->head;
  v1 = (Variable* )node->data;
  node = node->next;
  v2 = (Variable* )node->data;
/*
 * Add the seconds-interval to the date, and record the result for return.
 */
  DOUBLE_VARIABLE(result)->d = DOUBLE_VARIABLE(v1)->d +
    DOUBLE_VARIABLE(v2)->d / 86400.0;
  return 0;
}

/*.......................................................................
 * Define the date subtraction operator function. This is a binary
 * operator that expects a date value on its left and an interval value
 * on its right. The result is a new date.
 */
static OPER_FN(date_sub_fn)
{
  ListNode *node;      /* A node of the argument list */
  Variable *v1, *v2;   /* The input arguments */
  double utc;          /* The result of the subtraction */
/*
 * Get the two arguments.
 */
  node = args->head;
  v1 = (Variable* )node->data;
  node = node->next;
  v2 = (Variable* )node->data;
/*
 * Subtract the seconds-interval from the date.
 */
  utc = DOUBLE_VARIABLE(v1)->d - DOUBLE_VARIABLE(v2)->d / 86400.0;
/*
 * Record the result for return after applying limits.
 */
  DOUBLE_VARIABLE(result)->d = utc >= 0 ? utc : 0.0;
  return 0;
}

/*.......................................................................
 * This is a private do-loop iterator function used by Date datatypes.
 */
static DT_ITER(date_iterate)
{
  double a = DOUBLE_VARIABLE(first)->d;
  double b = DOUBLE_VARIABLE(last)->d;
  double inc = DOUBLE_VARIABLE(step)->d / 86400.0;
/*
 * Compute the number of steps required?
 */
  if(!value) {
    if(inc==0.0 || a==b || (b-a)/inc < 0.0)
      return 0;
    else
      return (int)(floor((b-a)/inc) + 1);
/*
 * Return the value for the latest iteration.
 */
  } else {
    DOUBLE_VARIABLE(value)->d = a + multiplier * inc;
    return 0;
  };
}

/*-----------------------------------------------------------------------*
 * Register                                                              *
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new register-specification datatype and add it to the
 * specified script environment.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name to give the datatype.
 *  regmap      RegMap *  The register map to lookup register
 *                        specifications in.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_RegisterDataType(Script *sc, char *name, ArrayMap *arraymap)
{
  DataType *dt;                   /* The object to be returned */
/*
 * Create the datatype and add it to the symbol table.
 */
  dt = new_DataType(sc, name, DT_BUILTIN, arraymap, sizeof(RegisterVariable),
		    0, 0, const_reg, print_reg, equal_reg, 0, in_reg, 0, NULL);
  if(!dt || !add_ScriptSymbol(sc, name, SYM_DATATYPE, dt))
    return NULL;
  return dt;
}

/*.......................................................................
 * Parse a register specification constant.
 */
static DT_CONST(const_reg)
{
  RegMap *regmap = (RegMap* )dt->context;  /* The register map */
  RegisterVariable *regvar;      /* The new variable */
  RegMapReg reg;                 /* The register specification */
/*
 * Read the register specification from the input stream.
 */
  if(input_RegMapReg(stream, 1, regmap, REG_INPUT_RANGE, 0, &reg))
    return 1;
/*
 * Record the specification in a new variable and push it onto the
 * expression stack.
 */
  regvar = (RegisterVariable *) new_Variable(sc, dt->atom_reg);
  if(!regvar || !add_LoadOper(sc, e, &regvar->v))
    return 1;
  regvar->board = reg.board;
  regvar->block = reg.block;
  regvar->index = reg.index;
  regvar->nreg = reg.nreg;
  return 0;
}

/*.......................................................................
 * Print a register specification variable.
 */
static DT_PRINT(print_reg)
{
  RegisterVariable *regvar;       /* The register variable to be printed */
  RegMapReg reg;                  /* A SZA register specification */
/*
 * The register map was passed to new_DataType() to be recorded in
 * the context member of the Register datatype. Retrieve it.
 */
  RegMap *regmap = (RegMap* )var->type->dt->context;
/*
 * Get the derived register variable.
 */
  regvar = REGISTER_VARIABLE(var);
/*
 * Convert the register specification into the form expected by
 * output_RegMapReg(), then display it.
 */
  if(init_RegMapReg(regmap, regvar->board, regvar->block, regvar->index,
		    regvar->nreg, REG_PLAIN, &reg) ||
     output_RegMapReg(output, regmap, REG_OUTPUT_RANGE, &reg))
    return 1;
  return 0;
}

/*.......................................................................
 * Return true if two register variables have the same values.
 */
static DT_RELFN(equal_reg)
{
/*
 * Get the register variables to be compared.
 */
  RegisterVariable *rva = REGISTER_VARIABLE(va);
  RegisterVariable *rvb = REGISTER_VARIABLE(vb);
/*
 * Compare the variables.
 */
  return rva->board==rvb->board && rva->block == rvb->block &&
         rva->index==rvb->index && rva->nreg == rvb->nreg;
}

/*.......................................................................
 * Return true if the second of two register specifications refers to
 * register elements that are covered by the first.
 */
static DT_RELFN(in_reg)
{
/*
 * Get the register variables to be compared.
 */
  RegisterVariable *rva = REGISTER_VARIABLE(va);
  RegisterVariable *rvb = REGISTER_VARIABLE(vb);
/*
 * Compare the variables.
 */
  return rva->board==rvb->board && rva->block == rvb->block &&
         rva->index <= rvb->index &&
         rva->index + rva->nreg >= rvb->index + rvb->nreg;
}

/*-----------------------------------------------------------------------*
 * Wdir                                                                  *
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype to contain path names of writable directories,
 * and add it to the specified script environment.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name to give the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_WdirDataType(Script *sc, char *name)
{
  return add_PathDataType(sc, name, check_wdir);
}

/*.......................................................................
 * Check whether the value of a wdir datatype specifies a writable
 * directory.
 *
 * Input:
 *  sc           Script *  The host scripting environment.
 *  var        Variable *  The variable who's value is to be checked.
 *  stream  InputStream *  The stream from which the directory name was
 *                         read.
 * Output:
 *  return       int    0 - Value ok.
 *                      1 - The string does not contain the name of a
 *                          directory that we have write access to.
 */
static DT_CHECK(check_wdir)
{
  char *path;         /* The path name in var */
/*
 * Get the pathname to be checked.
 */
  path = STRING_VARIABLE(var)->string;
/*
 * Check that the pathname refers to a directory.
 */
  if(test_pathname(path, PATH_IS_DIR, PATH_EXE | PATH_WRITE) != NULL) {
    input_error(stream, 1, "Error: '%s' is not a writable directory.\n", path);
    return 1;
  };
  return 0;
}
/*-----------------------------------------------------------------------*
 * Dev                                                                   *
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype to contain path names of devices
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name to give the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_DevDataType(Script *sc, char *name)
{
  return add_PathDataType(sc, name, NULL);
}

/*-----------------------------------------------------------------------*
 * Dir                                                                   *
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype to contain path names of directories to which
 * we have execute permission, and add it to the specified script
 * environment.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name to give the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_DirDataType(Script *sc, char *name)
{
  return add_PathDataType(sc, name, check_dir);
}

/*.......................................................................
 * Check whether the value of a dir datatype specifies an accessible
 * directory.
 *
 * Input:
 *  sc           Script *  The host scripting environment.
 *  var        Variable *  The variable who's value is to be checked.
 *  stream  InputStream *  The stream from which the directory name was
 *                         read.
 * Output:
 *  return       int    0 - Value ok.
 *                      1 - The string does not contain the name of a
 *                          directory that we have write access to.
 */
static DT_CHECK(check_dir)
{
  char *path;         /* The path name in var */
/*
 * Get the pathname to be checked.
 */
  path = STRING_VARIABLE(var)->string;
/*
 * Check that the pathname refers to a directory.
 */
  if(test_pathname(path, PATH_IS_DIR, PATH_EXE) != NULL) {
    input_error(stream, 1, "Error: '%s' is not an accessible directory.\n",
		path);
    return 1;
  };
  return 0;
}
/*-----------------------------------------------------------------------*      
 * IntTime                                                                      
 *-----------------------------------------------------------------------*/

/*.......................................................................       
 * Create a new datatype for specifying hardware integration times.             
 *                                                                              
 * Input:                                                                       
 *  sc          Script *  The target scripting environment.                     
 *  name          char *  The name to give the datatype.                        
 * Output:                                                                      
 *  return    DataType *  The newly added datatype, or NULL on error.  
*/
DataType *add_IntTimeDataType(Script *sc, char *name)
{
  return add_UintDataType(sc, name, check_inttime, sc_iterate_uint, "Integer");
}

/*.......................................................................       
 * Check whether the value of a "IntTime" datatype specifies a legal            
 * hardware accumulation interval. The interval is represented as a             
 * power of 2 exponent, and this must lie between                               
 * SCAN_MIN_HARDWARE_INTERVAL and SCAN_MAX_HARDWARE_INTERVAL. These             
 * values are enumerated in scanner.h.                                          
 *                                                                              
 * Input:                                                                       
 *  sc          Script *  The host scripting environment.                       
 *  var       Variable *  The variable who's value is to be checked.            
 *  stream InputStream *  The stream from which the variable value was          
 *                        read.                                                 
 * Output:                          
 *  return         int    0 - Value ok.                                         
 *                        1 - Out of bounds.                                    
 */
static DT_CHECK(check_inttime)
{
  unsigned interval = UINT_VARIABLE(var)->uint;
  if(interval < SCAN_MIN_HARDWARE_INTERVAL ||
     interval > SCAN_MAX_HARDWARE_INTERVAL) {
    input_error(stream, 1,
                "Legal hardware intervals are %u..%u. You asked for %u.\n",
                SCAN_MIN_HARDWARE_INTERVAL,
                SCAN_MAX_HARDWARE_INTERVAL,
                interval);
    return 1;
  };
  return 0;
}

/*-----------------------------------------------------------------------*
 * Board
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new register-board specification datatype and add it to the
 * specified script environment.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name to give the datatype.
 *  regmap      RegMap *  The register map to use to lookup board
 *                        names.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_BoardDataType(Script *sc, char *name, ArrayMap *arraymap)
{
  DataType *dt;                   /* The object to be returned */
/*
 * Create the datatype and add it to the symbol table.
 */
  dt = new_DataType(sc, name, DT_BUILTIN, arraymap, sizeof(UintVariable),
		    0, 0, const_board, print_board, sc_equal_uint,
		    0, 0, 0, NULL);
  if(!dt || !add_ScriptSymbol(sc, name, SYM_DATATYPE, dt))
    return NULL;
  return dt;
}

/*.......................................................................
 * Parse a register specification constant.
 */
static DT_CONST(const_board)
{
  Variable *var;        /* The variable that will contain the specification */
  RegMapBoard *brd;     /* The board description container */
/*
 * The register map was passed to new_DataType() to be recorded in
 * the context member of the Board datatype. Retrieve it.
 */
  RegMap *regmap = (RegMap* )dt->context;
/*
 * Read the board name from the input stream.
 */
  if(input_keyword(stream, 0, 1))
    return input_error(stream, 1, "Missing register-map board name.\n");
/*
 * Look up the named board.
 */
  brd = find_RegMapBoard(regmap, stream->work);
  if(!brd) {
    return input_error(stream, 1,
	       "Error: '%s' is not the name of a known register-map board.\n",
	       stream->work);
  };
/*
 * Record the specification in a new variable and push it onto the
 * expression stack.
 */
  var = new_Variable(sc, dt->atom_reg);
  if(!var || !add_LoadOper(sc, e, var))
    return 1;
/*
 * Inititialize the variable.
 */
  UINT_VARIABLE(var)->uint = brd->number;
  return 0;
}

/*.......................................................................
 * Print a register specification variable.
 */
static DT_PRINT(print_board)
{
/*
 * The register map was passed to new_DataType() to be recorded in
 * the context member of the Board datatype. Retrieve it.
 */
  RegMap *regmap = (RegMap* )var->type->dt->context;
/*
 * Get the board number.
 */
  int brd = UINT_VARIABLE(var)->uint;
/*
 * Print the name of the board.
 */
  return write_OutputStream(output, regmap->boards_[brd]->name);
}

/*-----------------------------------------------------------------------*
 * Time
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying times.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name to give the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_TimeDataType(Script *sc, char *name)
{
  //  return add_SexagesimalDataType(sc, name, check_time, time_iterate,
  //				 "Interval", 0);


  return add_Sexagesimal24DataType(sc, name, check_time, time_iterate,
				   "Interval", 0);
}

/*.......................................................................
 * Check the validity of a time variable.
 *
 * Input:
 *  sc          Script *  The host scripting environment.
 *  var       Variable *  The variable who's value is to be checked.
 *  stream InputStream *  The stream from which the variable value was
 *                        read.
 * Output:
 *  return         int    0 - Value ok.
 *                        1 - Out of bounds.
 */
static DT_CHECK(check_time)
{
  double t = DOUBLE_VARIABLE(var)->d;
  if(t >= 24 || t < 0.0) {
    input_error(stream, 1, "Invalid time-of-day. Should be 0 <= t < 24.\n");
    return 1;
  };
  return 0;
}

/*.......................................................................
 * This is a do-loop iterator function used by Antenna datatypes
 *
 * These are stored as unsigned bitmasks.  We will allow only
 * increments between single-element beginning states and end states,
 * otherwise it's not clear how you'd define increments between bitmasks.
 */
static DT_ITER(ant_iterate)
{
  unsigned a = SET_VARIABLE(first)->set;
  unsigned b = SET_VARIABLE(last)->set;
  unsigned incset = UINT_VARIABLE(step)->uint;
  int ibit,nbit = 32,anset=0,bnset=0,abit=0,bbit=0,incnset=0,inc=0;
#ifdef TEST
  inc = (int)incset;
#else
  for(ibit=0;ibit < nbit;ibit++) {

    if(incset & 1U<<ibit) 
      inc = ibit;

    if(a & 1U<<ibit) 
      abit = ibit;
  }
#endif
  /*
   * Compute the number of steps required?
   */
  if(!value) {
    /*
     * Check that the beginning and end states have only one bit set.
     */
    for(ibit=0;ibit < nbit;ibit++) {
#ifndef TEST
      if(incset & 1U<<ibit) 
	incnset++;
#endif
      if(a & 1U<<ibit) 
	anset++;
      if(b & 1U<<ibit) {
	bnset++;
	bbit = ibit;
      }
      
#ifdef TEST
      if(anset > 1 || bnset > 1 || incnset > 1) {
	lprintf(stderr,"Set variable iterators cannot have more than one bit set in the bitmask.\n");
	return 1;
      }
#else
      if(anset > 1 || bnset > 1) {
	lprintf(stderr,"Set variable iterators cannot have more than one bit set in the bitmask.\n");
	return 1;
      }
#endif
    }
    if(inc==0 || abit==bbit || (bbit-abit)/inc < 0.0)
      return 0;
    else
      return (int)(floor((double)(bbit-abit)/inc) + 1);
/*
 * Return the value for the latest iteration.
 */
  } else {
    SET_VARIABLE(value)->set = 1U<<(abit + (multiplier * inc));
    return 0;
  };
}

/*.......................................................................
 * This is a do-loop iterator function used by DDS datatypes
 *
 * These are stored as unsigned bitmasks.  We will allow only
 * increments between single-element beginning states and end states,
 * otherwise it's not clear how you'd define increments between bitmasks.
 */
static DT_ITER(dds_iterate)
{
  unsigned a = SET_VARIABLE(first)->set;
  unsigned b = SET_VARIABLE(last)->set;
  unsigned incset = UINT_VARIABLE(step)->uint;
  int ibit,nbit = 32,anset=0,bnset=0,abit=0,bbit=0,incnset=0,inc=0;

  for(ibit=0;ibit < nbit;ibit++) {

    if(incset & 1U<<ibit) 
      inc = ibit;

    if(a & 1U<<ibit) 
      abit = ibit;
  }

  /*
   * Compute the number of steps required?
   */
  if(!value) {
    /*
     * Check that the beginning and end states have only one bit set.
     */
    for(ibit=0;ibit < nbit;ibit++) {

      if(incset & 1U<<ibit) 
	incnset++;

      if(a & 1U<<ibit) 
	anset++;
      if(b & 1U<<ibit) {
	bnset++;
	bbit = ibit;
      }
      
      if(anset > 1 || bnset > 1) {
	lprintf(stderr,"Set variable iterators cannot have more than one bit set in the bitmask.\n");
	return 1;
      }

    }
    if(inc==0 || abit==bbit || (bbit-abit)/inc < 0.0)
      return 0;
    else
      return (int)(floor((double)(bbit-abit)/inc) + 1);
/*
 * Return the value for the latest iteration.
 */
  } else {
    SET_VARIABLE(value)->set = 1U<<(abit + (multiplier * inc));
    return 0;
  };
}

/*.......................................................................
 * This is a private do-loop iterator function used by Time datatypes.
 */
static DT_ITER(time_iterate)
{
  double a = DOUBLE_VARIABLE(first)->d;
  double b = DOUBLE_VARIABLE(last)->d;
  double inc = DOUBLE_VARIABLE(step)->d / 3600.0;  /* Seconds -> hours */
/*
 * Compute the number of steps required?
 */
  if(!value) {
    if(inc==0.0 || a==b || (b-a)/inc < 0.0)
      return 0;
    else
      return (int)(floor((b-a)/inc) + 1);
/*
 * Return the value for the latest iteration.
 */
  } else {
    DOUBLE_VARIABLE(value)->d = a + multiplier * inc;
    return 0;
  };
}

/*-----------------------------------------------------------------------*
 * Interval
 *-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
 * The Interval datatype is used to specify a time interval.
 *
 * It is stored as decimal days in a DoubleVariable. It supports the
 * standard arithmentic != == <= >= < > operators.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_IntervalDataType(Script *sc, char *name)
{
  DataType *dt;    /* The object to be returned */
  if(!sc || !name) {
    lprintf(stderr, "add_DayIntervalDataType: Invalid argument(s).\n");
    return NULL;
  };
  dt = new_DataType(sc, name, DT_BUILTIN, NULL, sizeof(DoubleVariable),
		    0, 0, const_interval, print_interval, sc_equal_double,
		    sc_gt_double, 0, sc_iterate_double, name);
  if(!dt || !add_ScriptSymbol(sc, name, SYM_DATATYPE, dt))
    return NULL;
  return dt;
}

/*.......................................................................
 * Parse an interval constant.
 */
static DT_CONST(const_interval)
{
  Variable *var;        /* The new day-interval constant */
  double secs;          /* The interval to record */
/*
 * Parse the interval.
 */
  if(input_interval(stream, 1, &secs))
    return 1;
/*
 * Record the interval in a new variable and push the variable onto the
 * expression stack.
 */
  var = new_Variable(sc, dt->atom_reg);
  if(!var || !add_LoadOper(sc, e, var))
    return 1;
/*
 * Initialize the value of the constant.
 */
  DOUBLE_VARIABLE(var)->d = secs;
  return 0;
}

/*.......................................................................
 * Print an interval variable.
 */
static DT_PRINT(print_interval)
{
  return output_interval(output, "-#", 0, 2, DOUBLE_VARIABLE(var)->d);
}

/*-----------------------------------------------------------------------*
 * Antennas
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying antenna selections.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_AntennasDataType(Script *sc, char *name)
{
  static Enumerator ant[] = {
    {"none",  gcp::util::AntNum::ANTNONE, ""},
    {"ant0",  gcp::util::AntNum::ANT0, ""}, 
    {"ant1",  gcp::util::AntNum::ANT1, ""}, 
    {"ant2",  gcp::util::AntNum::ANT2, ""}, 
    {"ant3",  gcp::util::AntNum::ANT3, ""},
    {"ant4",  gcp::util::AntNum::ANT4, ""},
    {"ant5",  gcp::util::AntNum::ANT5, ""},
    {"ant6",  gcp::util::AntNum::ANT6, ""},
    {"ant7",  gcp::util::AntNum::ANT7, ""},
    {   "0",  gcp::util::AntNum::ANT0, ""}, 
    {   "1",  gcp::util::AntNum::ANT1, ""}, 
    {   "2",  gcp::util::AntNum::ANT2, ""}, 
    {   "3",  gcp::util::AntNum::ANT3, ""},
    {   "4",  gcp::util::AntNum::ANT4, ""},
    {   "5",  gcp::util::AntNum::ANT5, ""},
    {   "6",  gcp::util::AntNum::ANT6, ""},
    {   "7",  gcp::util::AntNum::ANT7, ""},
    {"all",   gcp::util::AntNum::ANTALL, ""},
  };
  return add_SetDataType(sc, name, 0, ant, DIMENSION(ant), ant_iterate, 
			 "Antennas");
}

/*-----------------------------------------------------------------------*
 * SysType
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying the target systems of reboots
 * and shutdowns.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_SysTypeDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"cpu",        SYS_CPU, ""},
    {"rtc",        SYS_RTC, ""},
    {"control",    SYS_CONTROL, ""},
    {"pmac",       SYS_PMAC, ""},
  };
  return add_ChoiceDataType(sc, name, en, DIMENSION(en));
}

/*-----------------------------------------------------------------------*
 * TimeScale
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying time scales.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_TimeScaleDataType(Script *sc, char *name)
{
  static Enumerator type[] = {
    {"utc", TIME_UTC, ""},
    {"UTC", TIME_UTC, ""},
    {"lst", TIME_LST, ""},
    {"LST", TIME_LST, ""},
  };
  return add_ChoiceDataType(sc, name, type, DIMENSION(type));
}

/*-----------------------------------------------------------------------*
 * SwitchState                                                           *
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying hardware switch states.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_SwitchStateDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"on",  SWITCH_ON, ""},
    {"in",  SWITCH_ON, ""},
    {"off", SWITCH_OFF, ""},
    {"out", SWITCH_OFF, ""},
  };
  return add_ChoiceDataType(sc, name, en, DIMENSION(en));
}

/*-----------------------------------------------------------------------*
 * Source
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new source-specification datatype and add it to the
 * specified script environment.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name to give the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_SourceDataType(Script *sc, char *name)
{
  DataType *dt;                   /* The object to be returned */
/*
 * Create the datatype and add it to the symbol table.
 */
  dt = new_DataType(sc, name, DT_BUILTIN, NULL, sizeof(SourceVariable),
		    0, 0, const_source, print_source, equal_source, 0, 0,
		    0, NULL);
  if(!dt || !add_ScriptSymbol(sc, name, SYM_DATATYPE, dt))
    return NULL;
  return dt;
}

/*.......................................................................
 * Parse a source specification constant.
 */
static DT_CONST(const_source)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *var;   /* The new variable */
  SourceId id;     /* The identification header of the source */
  int length;      /* The length of the source name */
/*
 * Get the resource object of the navigator thread.
 */
  Navigator *nav = (Navigator* )cp_ThreadData(cp, CP_NAVIGATOR);
  int i;
/*
 * Read the source name from the input stream.
 */
  if(input_literal(stream, 0, "",  "",  valid_source_char, ""))
    return input_error(stream, 1, "Missing source name.\n");
/*
 * Check that the length of the name doesn't exceed the legal limit for
 * source names.
 */
  length = strlen(stream->work);
  if(length >= SRC_NAME_MAX) {
    return input_error(stream, 1,
		       "Source names can't be longer than %d characters.\n",
		       SRC_NAME_MAX - 1);
  };
/*
 * Convert the source name to lower case. The use of sc_equal_string()
 * as the equality method relies on this.
 */
  for(i=0; i<length; i++) {
    char *cptr = stream->work + i;
    if(isupper((int) *cptr))
      *cptr = tolower(*cptr);
  };
  
  // Look up the source to see whether it exists, but only if this is
  // not the current source.

  if(!navIsCurrent(stream->work))
    if(nav_lookup_source(nav, stream->work, 0, &id)) {
      return input_error(stream, 1,
			 "The source catalog does not contain an entry for"
			 " \"%s\".\n", stream->work);
    };
  
  // Create a new source variable and push it onto the expression
  // stack.

  var = new_Variable(sc, dt->atom_reg);
  if(!var || !add_LoadOper(sc, e, var))
    return 1;
  
  // Initialize the variable with the name of the source.

  strcpy(SOURCE_VARIABLE(var)->name, stream->work);
  return 0;
}

/*.......................................................................
 * Print a source specification variable.
 */
static DT_PRINT(print_source)
{
  return write_OutputStream(output, SOURCE_VARIABLE(var)->name);
}

/*.......................................................................
 * Test equality of source variables
 */
static DT_RELFN(equal_source)
{
  return strcmp(SOURCE_VARIABLE(va)->name, SOURCE_VARIABLE(vb)->name)==0;
}

/*-----------------------------------------------------------------------*
 * Scan
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new scan-specification datatype and add it to the
 * specified script environment.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name to give the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_ScanDataType(Script *sc, char *name)
{
  DataType *dt;                   /* The object to be returned */
/*
 * Create the datatype and add it to the symbol table.
 */
  dt = new_DataType(sc, name, DT_BUILTIN, NULL, sizeof(ScanVariable),
		    0, 0, const_scan, print_scan, equal_scan, 0, 0,
		    0, NULL);
  if(!dt || !add_ScriptSymbol(sc, name, SYM_DATATYPE, dt))
    return NULL;
  return dt;
}

/*.......................................................................
 * Parse a scan specification constant.
 */
static DT_CONST(const_scan)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  Variable *var;   /* The new variable */
  ScanId id;     /* The identification header of the scan */
  int length;      /* The length of the scan name */
/*
 * Get the resource object of the navigator thread.
 */
  Navigator *nav = (Navigator* )cp_ThreadData(cp, CP_NAVIGATOR);
  int i;
/*
 * Read the scan name from the input stream.
 */
  if(input_literal(stream, 0, "",  "",  valid_scan_char, ""))
    return input_error(stream, 1, "Missing scan name.\n");
/*
 * Check that the length of the name doesn't exceed the legal limit for
 * scan names.
 */
  length = strlen(stream->work);
  if(length >= SCAN_NAME_MAX) {
    return input_error(stream, 1,
		       "Scan names can't be longer than %d characters.\n",
		       SCAN_NAME_MAX - 1);
  };
/*
 * Convert the scan name to lower case. The use of sc_equal_string()
 * as the equality method relies on this.
 */
  for(i=0; i<length; i++) {
    char *cptr = stream->work + i;
    if(isupper((int) *cptr))
      *cptr = tolower(*cptr);
  };
/*
 * Look up the scan to see wether it exists.
 */
  if(nav_lookup_scan(nav, stream->work, 0, &id)) {
    return input_error(stream, 1,
		"The scan catalog does not contain an entry for \"%s\".\n",
		stream->work);
  };
/*
 * Create a new scan variable and push it onto the expression stack.
 */
  var = new_Variable(sc, dt->atom_reg);
  if(!var || !add_LoadOper(sc, e, var))
    return 1;
/*
 * Initialize the variable with the name of the scan.
 */
  strcpy(SCAN_VARIABLE(var)->name, stream->work);
  return 0;
}

/*.......................................................................
 * Print a scan specification variable.
 */
static DT_PRINT(print_scan)
{
  return write_OutputStream(output, SCAN_VARIABLE(var)->name);
}

/*.......................................................................
 * Test equality of scan variables
 */
static DT_RELFN(equal_scan)
{
  return strcmp(SCAN_VARIABLE(va)->name, SCAN_VARIABLE(vb)->name)==0;
}

/*-----------------------------------------------------------------------*
 * Model
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for selecting between optical and radio
 * pointing models.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_ModelDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"radio",     gcp::util::PointingMode::RADIO, "Selects the optical pointing model"},
    {"optical",   gcp::util::PointingMode::OPTICAL, "Selects the radio pointing model"},
  };
  return add_ChoiceDataType(sc, name, en, DIMENSION(en));
}

/*-----------------------------------------------------------------------*
 * Latitude
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying a latitude on the Earth.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_LatitudeDataType(Script *sc, char *name)
{
  return add_SexagesimalDataType(sc, name, check_latitude, sc_iterate_double,
				 "Sexagesimal", 1);
}

/*.......................................................................
 * Check the validity of a latitude variable.
 *
 * Input:
 *  sc          Script *  The host scripting environment.
 *  var       Variable *  The variable who's value is to be checked.
 *  stream InputStream *  The stream from which the variable value was
 *                        read.
 * Output:
 *  return         int    0 - Value ok.
 *                        1 - Out of bounds.
 */
static DT_CHECK(check_latitude)
{
  double latitude = DOUBLE_VARIABLE(var)->d;
  if(latitude < -90.0 || latitude > 90.0) {
    return input_error(stream, 1,
		       "Invalid latitude. Should be -90 <= latitude <= 90.\n");
  };
  return 0;
}

/*-----------------------------------------------------------------------*
 * Longitude
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying a longitude on the Earth.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_LongitudeDataType(Script *sc, char *name)
{
  return add_SexagesimalDataType(sc, name, check_longitude, sc_iterate_double,
				 "Sexagesimal", 1);
}

/*.......................................................................
 * Check the validity of a longitude variable.
 *
 * Input:
 *  sc          Script *  The host scripting environment.
 *  var       Variable *  The variable who's value is to be checked.
 *  stream InputStream *  The stream from which the variable value was
 *                        read.
 * Output:
 *  return         int    0 - Value ok.
 *                        1 - Out of bounds.
 */
static DT_CHECK(check_longitude)
{
  double longitude = DOUBLE_VARIABLE(var)->d;
  if(longitude < -180.0 || longitude > 180.0) {
    return input_error(stream, 1,
		"Invalid longitude. Should be -180 <= longitude <= 180.\n");
  };
  return 0;
}

/*-----------------------------------------------------------------------*
 * Azimuth
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying the target azimuth of the
 * telescope. The azimuths of the compass points are N=0, E=90, S=180
 * and W=270 degrees.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_AzimuthDataType(Script *sc, char *name)
{
  return add_SexagesimalDataType(sc, name, check_azimuth, sc_iterate_double,
				 "Sexagesimal", 1);
}

/*.......................................................................
 * Check the validity of an azimuth variable.
 *
 * Input:
 *  sc          Script *  The host scripting environment.
 *  var       Variable *  The variable who's value is to be checked.
 *  stream InputStream *  The stream from which the variable value was
 *                        read.
 * Output:
 *  return         int    0 - Value ok.
 *                        1 - Out of bounds.
 */
static DT_CHECK(check_azimuth)
{
  double azimuth = DOUBLE_VARIABLE(var)->d;
  if(azimuth < -360.0 || azimuth >= 360.0) {
    return input_error(stream, 1,
		"Invalid azimuth. Should be -360 <= azimuth < 360.\n");
  };
  return 0;
}

/*-----------------------------------------------------------------------*
 * DeckAngle
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying the position of the rotating
 * platform on which the dishes are mounted.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_DeckAngleDataType(Script *sc, char *name)
{
  return add_SexagesimalDataType(sc, name, check_deckangle, sc_iterate_double,
				 "Sexagesimal", 1);
}

/*.......................................................................
 * Check the validity of a deck-angle variable.
 *
 * Input:
 *  sc          Script *  The host scripting environment.
 *  var       Variable *  The variable who's value is to be checked.
 *  stream InputStream *  The stream from which the variable value was
 *                        read.
 * Output:
 *  return         int    0 - Value ok.
 *                        1 - Out of bounds.
 */
static DT_CHECK(check_deckangle)
{
  // We will not check this for Bicep, as any amount of travel is
  // allowed.  Will we ever check this for other experiments?  Ie,
  // should this be moved into specifictypes?

#if 0 
  double deck = DOUBLE_VARIABLE(var)->d;
  if(deck < -360.0 || deck > 360.0) {
    return input_error(stream, 1,
		       "Invalid DeckAngle. Should be -360 <= value <= 360.\n");
  };
#endif
  return 0;
}

/*-----------------------------------------------------------------------*
 * Elevation
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying the target elevation of the
 * telescope.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_ElevationDataType(Script *sc, char *name)
{
  return add_SexagesimalDataType(sc, name, check_elevation, sc_iterate_double,
				 "Sexagesimal", 1);
}

/*.......................................................................
 * Check the validity of an elevation variable.
 *
 * Input:
 *  sc          Script *  The host scripting environment.
 *  var       Variable *  The variable who's value is to be checked.
 *  stream InputStream *  The stream from which the variable value was
 *                        read.
 * Output:
 *  return         int    0 - Value ok.
 *                        1 - Out of bounds.
 */
static DT_CHECK(check_elevation)
{
  double elevation = DOUBLE_VARIABLE(var)->d;
  if(elevation < -90.0 || elevation > 90.0) {
    return input_error(stream, 1,
		       "Invalid elevation. Should be -90 <= el <= 90.\n");
  };
  return 0;
}

/*-----------------------------------------------------------------------*
 * Pointingoffset
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying temporary pointing offsets as
 * angles.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_PointingOffsetDataType(Script *sc, char *name)
{
  return add_SexagesimalDataType(sc, name, check_pointingoffset,
				 sc_iterate_double, "Sexagesimal", 1);
}

/*.......................................................................
 * Check the validity of a pointing-offset variable.
 *
 * Input:
 *  sc          Script *  The host scripting environment.
 *  var       Variable *  The variable who's value is to be checked.
 *  stream InputStream *  The stream from which the variable value was
 *                        read.
 * Output:
 *  return         int    0 - Value ok.
 *                        1 - Out of bounds.
 */
static DT_CHECK(check_pointingoffset)
{
  double offset = DOUBLE_VARIABLE(var)->d;
  if(offset < -360.0 || offset > 360.0) {
    return input_error(stream, 1,
	       "Invalid pointing offset. Should be -360 <= offset <= 360.\n");
  };
  return 0;
}

/*-----------------------------------------------------------------------*
 * Flexure
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying the degree of gravitational
 * drooping of the telescope as a function of elevation.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_FlexureDataType(Script *sc, char *name)
{
  return add_SexagesimalDataType(sc, name, check_flexure, sc_iterate_double,
				 "Sexagesimal", 1);
}

/*.......................................................................
 * Check the validity of a flexure variable.
 *
 * Input:
 *  sc          Script *  The host scripting environment.
 *  var       Variable *  The variable who's value is to be checked.
 *  stream InputStream *  The stream from which the variable value was
 *                        read.
 * Output:
 *  return         int    0 - Value ok.
 *                        1 - Out of bounds.
 */
static DT_CHECK(check_flexure)
{
  double flexure = DOUBLE_VARIABLE(var)->d;
  if(flexure <= -90.0 || flexure >= 90.0) {
    return input_error(stream, 1,
		       "Invalid flexure. Should be -90 < flexure < 90.\n");
  };
  return 0;
}

/*-----------------------------------------------------------------------*
 * Tilt
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying the tilts of the telescope axes.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_TiltDataType(Script *sc, char *name)
{
  return add_SexagesimalDataType(sc, name, check_tilt, sc_iterate_double,
				 "Sexagesimal", 1);
}

/*.......................................................................
 * Check the validity of a tilt variable.
 *
 * Input:
 *  sc          Script *  The host scripting environment.
 *  var       Variable *  The variable who's value is to be checked.
 *  stream InputStream *  The stream from which the variable value was
 *                        read.
 * Output:
 *  return         int    0 - Value ok.
 *                        1 - Out of bounds.
 */
static DT_CHECK(check_tilt)
{
  double tilt = DOUBLE_VARIABLE(var)->d;
  if(tilt <= -90.0 || tilt >= 90.0)
    return input_error(stream, 1, "Invalid tilt. Should be -90 < tilt < 90.\n");
  return 0;
}

/*-----------------------------------------------------------------------*
 * Tracking
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying the type of tracking
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_TrackingDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"both",  gcp::util::Tracking::TRACK_BOTH, "Both of the following:"},
    {"point", gcp::util::Tracking::TRACK_POINT, "Track a pointing center"},
    {"phase", gcp::util::Tracking::TRACK_PHASE, "Where relevant, track a phase center"},
  };
  return add_ChoiceDataType(sc, name, en, DIMENSION(en));
}

/*-----------------------------------------------------------------------*
 * Altitude
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying the height of the telescope above
 * the standard geodetic spheroid.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_AltitudeDataType(Script *sc, char *name)
{
  return add_DoubleDataType(sc, name, check_altitude, sc_iterate_double,
			    "Double", 1);
}

/*.......................................................................
 * Check the validity of an altitude variable. An altitude range is
 * needed so that the altitude can be encoded in a network long for
 * transmission to the real-time control system.
 *
 * Input:
 *  sc          Script *  The host scripting environment.
 *  var       Variable *  The variable who's value is to be checked.
 *  stream InputStream *  The stream from which the variable value was
 *                        read.
 * Output:
 *  return         int    0 - Value ok.
 *                        1 - Out of bounds.
 */
static DT_CHECK(check_altitude)
{
  double altitude = DOUBLE_VARIABLE(var)->d;
  if(altitude < -SZA_MAX_ALT || altitude > SZA_MAX_ALT) {
    return input_error(stream, 1,
		       "Invalid altitude. Should be %g <= altitude <= %g.\n",
		       -SZA_MAX_ALT, SZA_MAX_ALT);
  };
  return 0;
}

/*-----------------------------------------------------------------------*
 * GpibDev
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for selecting a device on the GPIB bus.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_GpibDevDataType(Script *sc, char *name)
{
  return add_UintDataType(sc, name, check_gpib_dev, 0, NULL);
}

/*.......................................................................
 * Check the validity of a gpib-device variable.
 *
 * Input:
 *  sc          Script *  The host scripting environment.
 *  var       Variable *  The variable who's value is to be checked.
 *  stream InputStream *  The stream from which the variable value was
 *                        read.
 * Output:
 *  return         int    0 - Value ok.
 *                        1 - Out of bounds.
 */
static DT_CHECK(check_gpib_dev)
{
  unsigned u = UINT_VARIABLE(var)->uint;
  if(u > 30)
    return input_error(stream, 1, "Invalid gpib device. Should be 0..30.\n");
  return 0;
}

/*-----------------------------------------------------------------------*
 * GpibCmd
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype to specify a command string to be sent to a
 * device on the GPIB bus.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_GpibCmdDataType(Script *sc, char *name)
{
  return add_StringDataType(sc, name, 1, check_gpib_cmd);
}

/*.......................................................................
 * Check the validity of a gpib command string.
 *
 * Input:
 *  sc          Script *  The host scripting environment.
 *  var       Variable *  The variable who's value is to be checked.
 *  stream InputStream *  The stream from which the variable value was
 *                        read.
 * Output:
 *  return         int    0 - Value ok.
 *                        1 - Out of bounds.
 */
static DT_CHECK(check_gpib_cmd)
{
  char *s = STRING_VARIABLE(var)->string;
  if(strlen(s) > GPIB_MAX_DATA)
    return input_error(stream, 1, "GPIB command string too long.\n");
  return 0;
}

/*-----------------------------------------------------------------------*
 * Features
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for enumerating frame features.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_FeaturesDataType(Script *sc, char *name)
{
  static Enumerator features[] = {
    {"f0",  1U<<0,""}, {"f1",  1U<<1,""}, {"f2",  1U<<2,""}, {"f3",  1U<<3,""},
    {"f4",  1U<<4,""}, {"f5",  1U<<5,""}, {"f6",  1U<<6,""}, {"f7",  1U<<7,""},
    {"f8",  1U<<8,""}, {"f9",  1U<<9,""}, {"f10",1U<<10,""}, {"f11",1U<<11,""},
    {"f12",1U<<12,""}, {"f13",1U<<13,""}, {"f14",1U<<14,""}, {"f15",1U<<15,""},
    {"f16",1U<<16,""}, {"f17",1U<<17,""}, {"f18",1U<<18,""}, {"f19",1U<<19,""},
    {"f20",1U<<20,""}, {"f21",1U<<21,""}, {"f22",1U<<22,""}, {"f23",1U<<23,""},
    {"f24",1U<<24,""}, {"f25",1U<<25,""}, {"f26",1U<<26,""}, {"f27",1U<<27,""},
    {"f28",1U<<28,""}, {"f29",1U<<29,""}, {"f30",1U<<30,""}, {"f31",1U<<31,""},
    {"all",   ~0U,""},
  };
  return add_SetDataType(sc, name, 1, features, DIMENSION(features), NULL, NULL);
}

/*-----------------------------------------------------------------------*
 * DeckMode
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying how to position the deck axis
 * while tracking a source.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_DeckModeDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"track",   DECK_TRACK, "Track in parallactic angle"},
    {"zero",    DECK_ZERO,  "Don't command the parallactic angle axis when tracking"},
  };
  return add_ChoiceDataType(sc, name, en, DIMENSION(en));
}

/*-----------------------------------------------------------------------*
 * Attenuation
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying attenuations over the range
 * 0..31db, in 1db steps.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_AttenuationDataType(Script *sc, char *name)
{
  return add_IntDataType(sc, name, check_attn_fn, sc_iterate_int, "Integer",1);
}

/*.......................................................................
 * Check the validity of an Attenuation variable.
 *
 * Input:
 *  sc          Script *  The host scripting environment.
 *  var       Variable *  The variable who's value is to be checked.
 *  stream InputStream *  The stream from which the variable value was
 *                        read.
 * Output:
 *  return         int    0 - Value ok.
 *                        1 - Out of bounds.
 */
static DT_CHECK(check_attn_fn)
{
  int i = INT_VARIABLE(var)->i;
  if(abs(i) > 31) {
    input_error(stream, 1, "Invalid attenuation. Should be -31 <= v <= 31.\n");
    return 1;
  };
  return 0;
}

/*-----------------------------------------------------------------------*
 * ArcFileType
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying an archive file type.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_ArcFileDataType(Script *sc, char *name)
{
  static Enumerator en[] = {  /* Note that ARC_***_FILE come from arcfile.h */
    {"log",     ARC_LOG_FILE, "The control system logfile"},
    {"archive", ARC_DAT_FILE, "The control system data archive"},
    {"grabber", ARC_GRAB_FILE, "Frames from the frame grabber"},
  };
  return add_ChoiceDataType(sc, name, en, DIMENSION(en));
}

/*-----------------------------------------------------------------------*
 * FeatureChangeType
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying what to do with a set of archive
 * feature markers.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_FeatureChangeDataType(Script *sc, char *name)
{
  static Enumerator en[] = {  /* Note that FEATURE_*** come from rtcnetcoms.h */
    {"add",     FEATURE_ADD,    "Add the requested feature bits"},
    {"remove",  FEATURE_REMOVE, "Remove the requested feature bits"},
    {"one",     FEATURE_ONE,    "Set the requested feature bits for a single frame"}
  };
  return add_ChoiceDataType(sc, name, en, DIMENSION(en));
}

/*-----------------------------------------------------------------------*
 * BitMask
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying a bit-mask as a set of 32 bits.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_BitMaskDataType(Script *sc, char *name)
{
  static Enumerator bits[] = {
    {"none",   0, ""},
    {"b0",  1U<<0,""}, {"b1",  1U<<1,""}, {"b2",  1U<<2,""}, {"b3",  1U<<3,""},
    {"b4",  1U<<4,""}, {"b5",  1U<<5,""}, {"b6",  1U<<6,""}, {"b7",  1U<<7,""},
    {"b8",  1U<<8,""}, {"b9",  1U<<9,""}, {"b10",1U<<10,""}, {"b11",1U<<11,""},
    {"b12",1U<<12,""}, {"b13",1U<<13,""}, {"b14",1U<<14,""}, {"b15",1U<<15,""},
    {"b16",1U<<16,""}, {"b17",1U<<17,""}, {"b18",1U<<18,""}, {"b19",1U<<19,""},
    {"b20",1U<<20,""}, {"b21",1U<<21,""}, {"b22",1U<<22,""}, {"b23",1U<<23,""},
    {"b24",1U<<24,""}, {"b25",1U<<25,""}, {"b26",1U<<26,""}, {"b27",1U<<27,""},
    {"b28",1U<<28,""}, {"b29",1U<<29,""}, {"b30",1U<<30,""}, {"b31",1U<<31,""},
    {"all",   ~0U,""},
  };
  return add_SetDataType(sc, name, 1, bits, DIMENSION(bits), NULL, NULL);
}

/*.......................................................................
 * Create a new datatype for enumerating bit-mask manipulation operators.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_BitMaskOperDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"set",     BIT_MASK_SET,    ""},
    {"clear",   BIT_MASK_CLEAR,  ""},
    {"assign",  BIT_MASK_ASSIGN, ""}
  };
  return add_ChoiceDataType(sc, name,  en, DIMENSION(en));
}

/*-----------------------------------------------------------------------*
 * Script
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * The Script datatype is used for entering the name and arguments
 * of a script.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name to give the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_ScriptDataType(Script *sc, char *name)
{
  DataType *dt;    /* The new datatype to be returned */
/*
 * Check arguments.
 */
  if(!sc || !name) {
    lprintf(stderr, "add_ScriptDataType: Invalid argument(s).\n");
    return NULL;
  };
/*
 * Create the datatype and add it to the symbol table.
 */
  dt = new_DataType(sc, name, DT_BUILTIN, NULL, sizeof(ScriptVariable),
		    0, 0, const_script, print_script, equal_script,
		    0, 0, 0, NULL);
  if(!dt || !add_ScriptSymbol(sc, name, SYM_DATATYPE, dt))
    return NULL;
  return dt;
}

/*.......................................................................
 * Parse a Script constant.
 */
static DT_CONST(const_script)
{
  Variable *var;   /* The new variable */
/*
 * Get the resource object of the parent thread.
 */
  Scheduler *sch = (Scheduler* )cp_ThreadData((ControlProg* )sc->project, 
					      CP_SCHEDULER);
  ScheduleData *sd = (ScheduleData* )sc->data;
/*
 * Create a new variable and push it onto the expression stack.
 */
  var = new_Variable(sc, dt->atom_reg);
  if(!var || !add_LoadOper(sc, e, var))
    return 1;
/*
 * Read the script path name as a literal string.
 */
  if(stream->nextc=='"' ?
     input_quoted_string(stream, 0) : 
     input_literal(stream,  1, "([{",  ")]}",  is_script_file, ""))
    return input_error(stream, 1, "Bad script file name.\n");
/*
 * Initialize the variable.
 */
  SCRIPT_VARIABLE(var)->sc = sch_compile_schedule(sch, "",stream->work, stream);
  if(!SCRIPT_VARIABLE(var)->sc)
    return 1;
/*
 * Keep a record of the compiled schedule so that we can delete it when
 * our parent script is discarded.
 */
  if(!append_ListNode(sd->schedules, SCRIPT_VARIABLE(var)->sc)) {
    del_Script(SCRIPT_VARIABLE(var)->sc);
    return 1;
  };
  return 0;
}

/*.......................................................................
 * Print the contents of a Script variable.
 */
static DT_PRINT(print_script)
{
  Script *script = SCRIPT_VARIABLE(var)->sc;
/*
 * Output the script name.
 */
  if(write_OutputStream(output, script->script.name))
    return 1;
/*
 * Are there any script arguments to display?
 */
  if(script->script.args->head) {
    if(write_OutputStream(output, "(") ||
       print_ArgumentList(script, output, 0, script->script.args) ||
       write_OutputStream(output, ")"))
      return 1;
  };
  return 0;
}

/*.......................................................................
 * Return non-zero if a character shouldn't be considered a part of a
 * script file name.
 */
static int is_script_file(int c)
{
  return c!=',' && c!='(' && c!=')' && c!='}' && c!= '\n';
}

/*.......................................................................
 * Test two Script variables for equality.
 */
static DT_RELFN(equal_script)
{
  return SCRIPT_VARIABLE(va)->sc == SCRIPT_VARIABLE(vb)->sc;
}

/*-----------------------------------------------------------------------*
 * OpticalCamera
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying the target of an optical camera 
 * command.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_OptCamTargetDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"frame",     OPTCAM_FRAME,      ""},
    {"controller",OPTCAM_CONTROLLER, ""},
    {"camera",    OPTCAM_CAMERA,     ""},
    {"stepper",   OPTCAM_STEPPER,    ""},
    {"focus",     OPTCAM_FOCUS,      ""},
    {"iris",      OPTCAM_IRIS,       ""},
    {"shutter",   OPTCAM_SHUT,       ""},
    {"sens_auto", OPTCAM_SENS_AUTO,  ""},
    {"sens_manu", OPTCAM_SENS_MANU,  ""},
    {"agc",       OPTCAM_AGC,        ""},
    {"alc",       OPTCAM_ALC,        ""},
    {"manu_iris", OPTCAM_MANU_IRIS,  ""},
    {"superd",    OPTCAM_SUPERD,     ""},
  };
  return add_ChoiceDataType(sc, name,  en, DIMENSION(en));
}

/*.......................................................................
 * Create a new datatype for specifying the action of an optical camera 
 * command.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_OptCamActionDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"id",      OPTCAM_ID, ""},
    {"rbc",     OPTCAM_RBC, ""},
    {"open",    OPTCAM_OPEN, ""},
    {"close",   OPTCAM_CLOSE, ""},
    {"preset",  OPTCAM_PRESET, ""},
    {"on",      OPTCAM_ON, ""},
    {"off",     OPTCAM_OFF, ""},
    {"inc",     OPTCAM_INC, ""},
    {"dec",     OPTCAM_DEC, ""},
    {"low",     OPTCAM_LOW, ""},
    {"mid",     OPTCAM_MID, ""},
    {"high",    OPTCAM_HIGH, ""},
    {"s100",    OPTCAM_100, ""},
    {"s250",    OPTCAM_250, ""},
    {"s500",    OPTCAM_500, ""},
    {"s1000",   OPTCAM_1000, ""},
    {"s2000",   OPTCAM_2000, ""},
    {"s4000",   OPTCAM_4000, ""},
    {"s10000",  OPTCAM_10000, ""},
    {"center",  OPTCAM_CENTER, ""}
  };
  return add_ChoiceDataType(sc, name,  en, DIMENSION(en));
}

/*-----------------------------------------------------------------------*
 * Peak datatype
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for enumerating frame grabber peak offsets
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_PeakDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"x",    PEAK_X,    ""},
    {"y",    PEAK_Y,    ""},
    {"xabs", PEAK_XABS, ""},
    {"yabs", PEAK_YABS, ""},
  };
  return add_ChoiceDataType(sc, name,  en, DIMENSION(en));
}

/*.......................................................................
 * Create a new datatype for enumerating frame grabber image statistics.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_ImstatDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"peak", IMSTAT_PEAK, "The peak pixel value"},
    {"snr",  IMSTAT_SNR,  "The signal-to-noise of the peak pixel value (peak/rms)"},
  };
  return add_ChoiceDataType(sc, name,  en, DIMENSION(en));
}
/*-----------------------------------------------------------------------*
 * OptCamCount
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying stepper motor integer steps.
 * These are stored as integers between -10500 and 10500 (supposedly, full 
 * range is ~ +- 5200). In addition, stepper count variables will 
 * contain -20000 when the user specifies the count as "off", and +20000 
 * when the user specifies the count as "on".
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_OptCamCountDataType(Script *sc, char *name)
{
  DataType *dt;    /* The object to be returned */
  if(!sc || !name) {
    lprintf(stderr, "add_OptCamCount: Invalid argument(s).\n");
    return NULL;
  };
/*
 * Create the datatype and add it to the symbol table.
 */
  dt = new_DataType(sc, name, DT_BUILTIN, NULL, sizeof(IntVariable),
		    0, 0, const_optcam, print_optcam, equal_optcam, 
		    gt_optcam, 0, 0, NULL);
  if(!dt || !add_ScriptSymbol(sc, name, SYM_DATATYPE, dt))
    return NULL;
  return dt;
}

/*.......................................................................
 * Parse an optical camera constant.  The stepper motor has full range
 * +-26 turns x 200 steps/turn = +-5200 steps.
 */
static DT_CONST(const_optcam)
{
  char *usage = "Expected a valid step count or a valid camera control keyword.\n";
  Variable *var;        /* The variable that will contain the number */
  int l;               /* The floating point read from the input stream */
  int i;
/*
 * Read either an integral count, or an alphanumeric keyword, from the input
 * stream.
 */
  if(isalpha(stream->nextc)) {
    if(input_keyword(stream, 0, 1))
      return input_error(stream, 1, usage);
    /*
     * Loop through the enumerated list, looking for a match.
     */
    for(i=0;i < OptCamNen;i++) {
      if(strcmp(stream->work, OptCamEn[i].name)==0) {
	l = (int) OptCamEn[i].value; /* Pass the OptCamAction as an integer. */
	break;
      }
    }
    /*
     * If no match was found, return error.
     */
    if(i==OptCamNen)
      return input_error(stream, 1, usage);
  }
  else {
    if(input_int(stream, 0, 0, &l))
      return input_error(stream, 1, usage);
    /*
     * Check the domain of the variable.
     */
    if(l < -10500 || l > 10500) {
      return input_error(stream, 1,
			 "Invalid count. Should be -10500 - +10500.\n");
    };
  };
/*
 * Record the number in a new variable and push that variable onto the
 * expression stack.
 */
  var = new_Variable(sc, dt->atom_reg);
  if(!var || !add_LoadOper(sc, e, var))
    return 1;
/*
 * Initialize the variable.
 */
  INT_VARIABLE(var)->i = l;
  return 0;
}
/*.......................................................................
 * Print an optical camera count.
 */
static DT_PRINT(print_optcam)
{
  int l = INT_VARIABLE(var)->i;
  int i;
  /*
   * If less than 20000, this is an integral count.
   */
  if(l < 20000)
    return output_int(output, OUT_DECIMAL, "+", 0, 0, l);
  /*
   * Else loop through the Enumerated list, looking for a match.
   */
  else {
    for(i=0; i < OptCamNen; i++)
      if((OptCamAction)l == (int)OptCamEn[i].value)
	return write_OutputStream(output, OptCamEn[i].name);
  }
  /*
   * Else this was not a recognized count.
   */
  lprintf(stderr,"Unrecognized OptCamCount value.\n");
  return 1;
}
/*.......................................................................
 * Test two optical camera counts for equality.
 */
static DT_RELFN(equal_optcam)
{
  return INT_VARIABLE(va)->i == INT_VARIABLE(vb)->i;
}
/*.......................................................................
 * Test whether the value of an optical camera count variable is greater than a
 * second. Note that "off" is reported as less than any
 * other optical camera value, and "on" is reported as greater.
 */
static DT_RELFN(gt_optcam)
{
  return INT_VARIABLE(va)->i > INT_VARIABLE(vb)->i;
}

/*-----------------------------------------------------------------------*
 * SlewRate
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new datatype for specifying a reduced slew rate.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_SlewRateDataType(Script *sc, char *name)
{
  return add_UintDataType(sc, name, check_slew_rate, sc_iterate_uint,"Integer");
}

/*.......................................................................
 * Check the validity of a slew-rate variable.
 *
 * Input:
 *  sc          Script *  The host scripting environment.
 *  var       Variable *  The variable who's value is to be checked.
 *  stream InputStream *  The stream from which the variable value was
 *                        read.
 * Output:
 *  return         int    0 - Value ok.
 *                        1 - Out of bounds.
 */
static DT_CHECK(check_slew_rate)
{
  unsigned u = UINT_VARIABLE(var)->uint;
  if(u < 10 || u > 100) {
    input_error(stream, 1, "Invalid percentage slew-rate. Should be 10 <= r <= 100.\n");
    return 1;
  };
  return 0;
}

/*-----------------------------------------------------------------------*
 * EmailAction                                                           *
 *-----------------------------------------------------------------------*/

/*......................................................................
 * Create a new datatype for specifying an action to perform with a
 * pager email address
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_EmailActionDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"add",      EMAIL_ADD,   "Add an email address to the list to be notified when the pager is activated"},
    {"clear",    EMAIL_CLEAR, "Clear the email list"},
    {"list",     EMAIL_LIST,  "Display the current email list"},
  };
  return add_ChoiceDataType(sc, name, en, DIMENSION(en));
}

/*-----------------------------------------------------------------------*
 * PagerState                                                           *
 *-----------------------------------------------------------------------*/

/*......................................................................
 * Create a new datatype for specifying the state of the pager
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_PagerStateDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"on",     PAGER_ON,      "Activate the pager"},
    {"off",    PAGER_OFF,     "Deactivate the pager "
                              "(for sustained paging devices"},
    {"chip",   PAGER_IP,      ""},
    {"email",  PAGER_EMAIL,   ""},
    {"disable",PAGER_DISABLE, "Disable the pager altogether"},
    {"enable", PAGER_ENABLE,  "Enable the pager"},
    {"clear",  PAGER_CLEAR},
    {"list",   PAGER_LIST},
    {"status", PAGER_STATUS,  "Get a status report from the modem pager"},

  };
  return add_ChoiceDataType(sc, name, en, DIMENSION(en));
}

/*-----------------------------------------------------------------------*
 * PagerDev                                                              *
 *-----------------------------------------------------------------------*/

/*......................................................................
 * Create a new datatype for specifying a pager device
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_PagerDevDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"mapo_redlight", PAGER_MAPO_REDLIGHT, ""},
    {"dome_pager",    PAGER_DOME_PAGER, ""},
    {"eldorm_pager",  PAGER_ELDORM_PAGER, ""},
  };
  return add_ChoiceDataType(sc, name, en, DIMENSION(en));
}

/*-----------------------------------------------------------------------*
 * DelayType
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a datatype for specifying a delay
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_DelayTypeDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"fixed",        FIXED, ""},
    {"adjustable",   ADJUSTABLE, ""},
    {"geometric",    GEOMETRIC, ""},
    {"thermal",      THERMAL, ""},
    {"ionospheric",  IONOSPHERIC, ""},
    {"tropospheric", TROPOSPHERIC, ""},
  };
  return add_ChoiceDataType(sc, name, en, DIMENSION(en));
}

/*-----------------------------------------------------------------------*
 * TransDev
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new transdev-specification datatype and add it to the
 * specified script environment.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name to give the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_TransDevDataType(Script *sc, char *name)
{
  DataType *dt;                   /* The object to be returned */
/*
 * Create the datatype and add it to the symbol table.
 */
  dt = new_DataType(sc, name, DT_BUILTIN, NULL, sizeof(TransDevVariable),
		    0, 0, const_transdev, print_transdev, equal_transdev, 0, 0,
		    0, NULL);
  if(!dt || !add_ScriptSymbol(sc, name, SYM_DATATYPE, dt))
    return NULL;
  return dt;
}

/*.......................................................................
 * Parse a transdev specification constant.
 */
static DT_CONST(const_transdev)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   retransdev object */
  Variable *var;   /* The new variable */
  int length;      /* The length of the transdev name */
  
  // Get the resource object of the logger thread.

  SzaArrayLogger *log = (SzaArrayLogger* )cp_ThreadData(cp, CP_LOGGER);
  
  // Read the transdev name from the input stream.

  if(input_literal(stream, 0, "",  "",  TransactionManager::notSeparatorChar, ""))
    return input_error(stream, 1, "Missing transdev name.\n");
  
  // Check that the length of the name doesn't exceed the legal limit
  // for transdev names.

  length = strlen(stream->work);
  if(length >= (int)TransactionManager::DEV_NAME_MAX) {
    return input_error(stream, 1,
		       "TransDev names can't be longer than %d characters.\n",
		       TransactionManager::DEV_NAME_MAX);
  };
  
  // Look up the device to see whether it exists, but only if this is
  // not the current transdev.

  if(!log_isValidDevice(log, stream->work)) {
    return input_error(stream, 1,
		       "The transaction catalog does not contain an entry for"
		       " \"%s\".\n", stream->work);
  };
  
  // Create a new TransVar variable and push it onto the expression
  // stack.

  var = new_Variable(sc, dt->atom_reg);
  if(!var || !add_LoadOper(sc, e, var))
    return 1;
  
  // Initialize the variable with the name of the device

  strcpy(TRANSDEV_VARIABLE(var)->name, stream->work);
  return 0;
}

/*.......................................................................
 * Print a transdev specification variable.
 */
static DT_PRINT(print_transdev)
{
  return write_OutputStream(output, TRANSDEV_VARIABLE(var)->name);
}

/*.......................................................................
 * Test equality of transdev variables
 */
static DT_RELFN(equal_transdev)
{
  return strcmp(TRANSDEV_VARIABLE(va)->name, TRANSDEV_VARIABLE(vb)->name)==0;
}

/*-----------------------------------------------------------------------*
 * TransLocation
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new location-specification datatype and add it to the
 * specified script environment.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name to give the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_TransLocationDataType(Script *sc, char *name)
{
  return add_StringDataType(sc, name, 0, NULL);
}

/*-----------------------------------------------------------------------*
 * TransSerial
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new serial-number specification datatype and add it to the
 * specified script environment.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name to give the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_TransSerialDataType(Script *sc, char *name)
{
  return add_StringDataType(sc, name, 0, NULL);
}

/*-----------------------------------------------------------------------*
 * TransWho
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new specification datatype for specifying the person
 * logging a transaction and add it to the specified script
 * environment.
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name to give the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_TransWhoDataType(Script *sc, char *name)
{
  return add_StringDataType(sc, name, 0, NULL);
}

/*-----------------------------------------------------------------------*
 * TransWho
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Create a new specification datatype for an optional transaction
 * comment
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name to give the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_TransCommentDataType(Script *sc, char *name)
{
  return add_StringDataType(sc, name, 0, NULL);
}

/*-----------------------------------------------------------------------
 * The ImDir datatype is used to select the orientation of the frame
 * grabber image axes
 */ 
DataType *add_ImDirDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"upright",  gcp::control::UPRIGHT,  "The axis is displayed in the conventional orientation"},
    {"normal",   gcp::control::UPRIGHT,  "The axis is displayed in the conventional orientation"},
    {"inverted", gcp::control::INVERTED, "The axis is inverted w.r.t. the conventional orientation"},
  };
  return add_ChoiceDataType(sc, name, en, DIMENSION(en));
}

/*-----------------------------------------------------------------------
 * The RotSense datatype is used to select the sens of the deck
 * rotation.
 */ 
DataType *add_RotSenseDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"cw",                  gcp::control::CW, ""},
    {"clockwise",           gcp::control::CW, ""},
    {"ccw",                 gcp::control::CCW, ""},
    {"counter-clockwise",   gcp::control::CCW, ""},
  };
  return add_ChoiceDataType(sc, name, en, DIMENSION(en));
}

/*.......................................................................
 * Create a new datatype for specifying the mode of the frame grabber
 * flatfielding
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_FlatfieldModeDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"none",  gcp::grabber::Flatfield::FLATFIELD_NONE,  "No flatfielding should be done"},
    {"row",   gcp::grabber::Flatfield::FLATFIELD_ROW,   "A mean is subtracted from each pixel row"},
    {"image", gcp::grabber::Flatfield::FLATFIELD_IMAGE, "The image is divided by a flatfield image.  Use in conjunction with the 'takeFlatfield()' command"}
  };
  return add_ChoiceDataType(sc, name, en, DIMENSION(en));
}

/*.......................................................................
 * Create a new datatype for specifying frame grabber channel selection
 */
DataType *add_FgChannelDataType(Script *sc, char *name)
{
  static Enumerator chans[] = {
    { "none",  gcp::grabber::Channel::NONE, "no channels"},

    {"chan0",  gcp::grabber::Channel::CHAN0, "channel 0"}, 
    {"chan1",  gcp::grabber::Channel::CHAN1, "channel 1"}, 
    {"chan2",  gcp::grabber::Channel::CHAN2, "channel 2"}, 
    {"chan3",  gcp::grabber::Channel::CHAN3, "channel 3"}, 

    {    "0",  gcp::grabber::Channel::CHAN0, "channel 0"}, 
    {    "1",  gcp::grabber::Channel::CHAN1, "channel 1"}, 
    {    "2",  gcp::grabber::Channel::CHAN2, "channel 2"}, 
    {    "3",  gcp::grabber::Channel::CHAN3, "channel 3"}, 

    {  "all",  gcp::grabber::Channel::ALL,   "all channels"},
  };

  return add_SetDataType(sc, name, 0, chans, DIMENSION(chans), NULL, "FgChannel");
}

/*.......................................................................
 * Create a new datatype for specifying the targets of pointing telescope commands
 *
 * Input:
 *  sc          Script *  The target scripting environment.
 *  name          char *  The name for the datatype.
 * Output:
 *  return    DataType *  The newly added datatype, or NULL on error.
 */
DataType *add_PointingTelescopesDataType(Script *sc, char *name)
{
  static Enumerator en[] = {
    {"ptel0",    gcp::util::PointingTelescopes::PTEL_ZERO},
    {"ptel1",    gcp::util::PointingTelescopes::PTEL_ONE},
    {"ptel2",    gcp::util::PointingTelescopes::PTEL_TWO},
    {"0",        gcp::util::PointingTelescopes::PTEL_ZERO},
    {"1",        gcp::util::PointingTelescopes::PTEL_ONE},
    {"2",        gcp::util::PointingTelescopes::PTEL_TWO},
    {"all",      gcp::util::PointingTelescopes::PTEL_ALL},
  };
  return add_SetDataType(sc, name, 0, en, DIMENSION(en), 0, 0);
}

