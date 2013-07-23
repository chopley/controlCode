#include <iostream>

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "SpecificTransactionStatus.h"
#include "genericcontrol.h"
#include "specificscript.h"
#include "genericregs.h"
#include "specifictypes.h"
#include "genericscheduler.h"
#include "const.h"
#include "navigator.h"
#include "arcfile.h"
#include "pathname.h"
#include "grabber.h"
#include "archiver.h"

using namespace gcp::control;
using namespace std;

// Generic Command Execution

static CMD_FN(sc_exec_cmd);
static CMD_FN(sc_setFilter_cmd);
static CMD_FN(sc_scriptDir_cmd);

// Cryo Con Commands

static CMD_FN(sc_setUpCryoLoop_cmd);
static CMD_FN(sc_heatUpCryoSensor_cmd);
static CMD_FN(sc_resumeCryoCooling_cmd);
static CMD_FN(sc_resetCryoModule_cmd);
static CMD_FN(sc_stopCryoControlLoop_cmd);
static CMD_FN(sc_engageCryoControlLoop_cmd);
static CMD_FN(sc_setCryoSkyTemp_cmd);
static CMD_FN(sc_setCryoSourceChannel_cmd);
static CMD_FN(sc_setCryoLoopRange_cmd);
static CMD_FN(sc_setCryoPGain_cmd);
static CMD_FN(sc_setCryoIGain_cmd);
static CMD_FN(sc_setCryoDGain_cmd);
static CMD_FN(sc_setCryoPowerOutput_cmd);
static CMD_FN(sc_setCryoHeaterLoad_cmd);
static CMD_FN(sc_setCryoControlType_cmd);

// Servo Specific Commands
static CMD_FN(sc_servoInitializeAntenna_cmd);
static CMD_FN(sc_setServoLoopParameters_cmd);
static CMD_FN(sc_engageServo_cmd);
static CMD_FN(sc_enableClutches_cmd);
static CMD_FN(sc_enableBrakes_cmd);
static CMD_FN(sc_enableContactors_cmd);

// Rx Backend Specific Commands
static CMD_FN(sc_setupAdc_cmd);
static CMD_FN(sc_resetFpga_cmd);
static CMD_FN(sc_resetFifo_cmd);
static CMD_FN(sc_setBurstLength_cmd);
static CMD_FN(sc_setSwitchPeriod_cmd); //gone
static CMD_FN(sc_setIntegrationPeriod_cmd); //gone
static CMD_FN(sc_setTrimLength_cmd);  
static CMD_FN(sc_enableRxSimulator_cmd);
static CMD_FN(sc_enableRxNoise_cmd);
static CMD_FN(sc_enableWalshing_cmd);
static CMD_FN(sc_enableAltWalshing_cmd);
static CMD_FN(sc_enableFullWalshing_cmd);
static CMD_FN(sc_enableNonLinearity_cmd);
static CMD_FN(sc_getBurstData_cmd);
static CMD_FN(sc_enableAlphaCorrection_cmd);  
static CMD_FN(sc_setAlphaCorrection_cmd);  
static CMD_FN(sc_setNonlinCorrection_cmd);  

// ROACH BOARD COMMAND
static CMD_FN(sc_generalRoachCommand_cmd);  


// LNA power supply commands
static CMD_FN(sc_setDrainVoltage_cmd);
static CMD_FN(sc_setGateVoltage_cmd);
static CMD_FN(sc_setDrainCurrent_cmd);
static CMD_FN(sc_setBias_cmd);
static CMD_FN(sc_setModule_cmd);
static CMD_FN(sc_changeVoltage_cmd);
static CMD_FN(sc_getVoltage_cmd);
static CMD_FN(sc_enableBiasQuery_cmd);


// Event management

static FUNC_FN(sc_acquired_fn);



/*.......................................................................
 * Create a new SZA scripting environment.
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
Script* add_SpecificScriptCommands(ControlProg *cp, Script* sc, int batch, 
				   HashTable *signals)
{
  if(!sc)
    return NULL;
  
  if(!add_AcquireTargetsDataType(sc, "AcquireTargets") ||
     !add_ShutterStateDataType(sc, "ShutterState") ||
     !add_HeaterTempDataType(sc, "HeaterTemp") ||
     !add_LnaNumberDataType(sc, "Lna") ||
     !add_LnaStageNumberDataType(sc, "LnaStage") ||
     !add_RoachNumberDataType(sc, "RoachNumber") ||
     !add_LnaDrainCurrentDataType(sc, "Id") ||
     !add_LnaDrainVoltageDataType(sc, "Vd") ||
     !add_RxStageDataType(sc, "Stage") ||
     !add_RxChannelDataType(sc, "Channel") ||
     !add_HeaterPIDDataType(sc, "HeaterPID") ||
     !add_HeaterStateDataType(sc, "HeaterState") ||
     !add_BuiltinFunction(sc, "Boolean acquired(AcquireTargets targets, "
			  "[Antennas ant])",
			  sc_acquired_fn) ||
     !add_StringDataType(sc, "Command", 0, 0) ||
     !add_BuiltinCommand(sc, "exec (Command command)", sc_exec_cmd))
      return del_Script(sc);
  
  // Add the command to set the filter function
  
  if(!add_BuiltinCommand(sc, "setFilter([Double freqHz, Integer ntap])", 
                         sc_setFilter_cmd))
     return del_Script(sc);

  // Add the command to set the script directory
  
  if(!add_BuiltinCommand(sc, "scriptDir(Wdir dir)",
                         sc_scriptDir_cmd))
     return del_Script(sc);

  //------------------------------------------------------------
  // Add Cryocon commands
  //------------------------------------------------------------
  
  if(!add_BuiltinCommand(sc, "setUpCryoLoop(Integer loopNum)",
                         sc_setUpCryoLoop_cmd, "Sets up the Cryo Loop Parameters to their defaul values") || 

     !add_BuiltinCommand(sc, "heatUpCryoSensor(Integer loopNum)",
                         sc_heatUpCryoSensor_cmd, "Heats up the Cryo Sensor") ||

     !add_BuiltinCommand(sc, "resumeCryoCooling(Integer loopNum)",
                         sc_resumeCryoCooling_cmd, "Resume the cooling of the Cryo Sensor") ||

     !add_BuiltinCommand(sc, "resetCryoModule()",
                         sc_resetCryoModule_cmd, "Reset the CryoCon") ||

     !add_BuiltinCommand(sc, "stopCryoControlLoop()",
                         sc_stopCryoControlLoop_cmd, "Stop the control Loop in the Cryocon") ||

     !add_BuiltinCommand(sc, "engageCryoControlLoop()",
                         sc_engageCryoControlLoop_cmd, "Engage the control Loop in the Cryocon") ||

     !add_BuiltinCommand(sc, "setCryoSkyTemp(Integer loopNum, Double Value)", 
                         sc_setCryoSkyTemp_cmd, "Set the Sky Temperature in the Cryocon loop") ||

     !add_BuiltinCommand(sc, "setCryoSourceChannel(Integer loopNum, Integer Channel)", 
                         sc_setCryoSourceChannel_cmd, "Set the Source Channel for control Loop (1 (ch A), or 2 (ch B))") || 

     !add_BuiltinCommand(sc, "setCryoLoopRange(Integer loopNum, Integer Range)", 
                         sc_setCryoLoopRange_cmd, "Set the Cryo Loop Range (0 (low), 1 (mid), or 2(high) )") ||

     !add_BuiltinCommand(sc, "setCryoPGain(Integer loopNum, Double Value)", 
                         sc_setCryoPGain_cmd, "Set the Proportional Gain in the PID Cryocon loop") ||

     !add_BuiltinCommand(sc, "setCryoIGain(Integer loopNum, Double Value)", 
                         sc_setCryoIGain_cmd, "Set the Integral Gain in the PID Cryocon loop") ||

     !add_BuiltinCommand(sc, "setCryoDGain(Integer loopNum, Double Value)", 
                         sc_setCryoDGain_cmd, "Set the Differential Gain in the PID Cryocon loop") ||

     !add_BuiltinCommand(sc, "setCryoPowerOutput(Integer loopNum, Double val)",
                         sc_setCryoPowerOutput_cmd, "Set the Cryocon output power level") ||

     !add_BuiltinCommand(sc, "setCryoHeaterLoad(Integer loopNum, Double val)",
                         sc_setCryoHeaterLoad_cmd, "Set the Cryocon heater load") ||

     !add_BuiltinCommand(sc, "setCryoControlType(Integer loopNum, Integer value)",
                         sc_setCryoControlType_cmd, "Set the Cryocon control type (0(off), 1(PID), 2(Manual), 3(Table), or 4(RampP))"))

        return del_Script(sc);

  //------------------------------------------------------------
  // Add Servo Commands
  //------------------------------------------------------------
  
  // add data types for servo
  if(!add_AxisDataType(sc, "Axis"))
    return del_Script(sc);


    if(
       !add_BuiltinCommand(sc, "servoInitializeAntenna()",
			   sc_servoInitializeAntenna_cmd, "Initializes the antenna, including calibration of the encoders") ||
       
       !add_BuiltinCommand(sc, "setServoLoopParameters(Integer loopNum, Double val1, Double val2, Double val3, Double val4, Double val5, Double val6, Double val7)", 
			   sc_setServoLoopParameters_cmd, "Sets the loop parameters for the Servo.  LoopNum is 0 for all, then 1-4 for the rest") ||

       !add_BuiltinCommand(sc, "engageServo(SwitchState state)",
			   sc_engageServo_cmd, "Engages (or disengages) the drives"),

       !add_BuiltinCommand(sc, "enableClutches(SwitchState state)",
			   sc_enableClutches_cmd, "Engages (or disengages) the clutches") ||

       !add_BuiltinCommand(sc, "enableBrakes(SwitchState state, Axis axVal )",
			   sc_enableBrakes_cmd, "Turns brakes on/off for a given axis")  ||

       !add_BuiltinCommand(sc, "enableContactors(SwitchState state, Axis axVal)",
			   sc_enableContactors_cmd, "Turns contactors on/off for a given axis")
       )
      return del_Script(sc);


  //------------------------------------------------------------
  // Add Rx Backend Commands
  //------------------------------------------------------------
  
    if(
       !add_BuiltinCommand(sc, "setupAdc()",
			   sc_setupAdc_cmd, "Sets up the ADC") ||

       !add_BuiltinCommand(sc, "resetFpga()",
			   sc_resetFpga_cmd, "Resets the memory on the FPGA") ||

       !add_BuiltinCommand(sc, "resetFifo()",
			   sc_resetFifo_cmd, "Resets the memory on the FIFO") ||

       !add_BuiltinCommand(sc, "setBurstLength(Double value)",
			   sc_setBurstLength_cmd, "Sets the length of the burst") ||

       !add_BuiltinCommand(sc, "setSwitchPeriod(Double value)",
			   sc_setSwitchPeriod_cmd, "Sets the length of Walshing period") ||

       !add_BuiltinCommand(sc, "setIntegrationPeriod(Double value)",
			   sc_setIntegrationPeriod_cmd, "Sets the integration period") ||

       !add_BuiltinCommand(sc, "setTrimLength(Double value)",
			   sc_setTrimLength_cmd, "Sets the number of values to trim around the Walsh switch (0 to 255)") ||

       !add_BuiltinCommand(sc, "enableRxSimulator(Boolean value)",
			   sc_enableRxSimulator_cmd, "Enables the backend simulator mode") ||

       !add_BuiltinCommand(sc, "enableRxNoise(Boolean value)",
			   sc_enableRxNoise_cmd, "Enables the backend noise source") ||

       !add_BuiltinCommand(sc, "enableWalshing(Boolean value)",
			   sc_enableWalshing_cmd, "Enables the backend Walshing functions") ||

       !add_BuiltinCommand(sc, "enableAltWalshing(Boolean value)",
			   sc_enableAltWalshing_cmd, "Enables the alternate Walshing functions") ||

       !add_BuiltinCommand(sc, "enableFullWalshing(Boolean value)", 
			   sc_enableFullWalshing_cmd, "Enables the full Walshing functions") ||

       !add_BuiltinCommand(sc, "enableNonLinearity(Boolean value)",
			   sc_enableNonLinearity_cmd, "Enables the non-linearity corrections in the backend") ||

       !add_BuiltinCommand(sc, "getBurstData()",
			   sc_getBurstData_cmd, "Gets data from the burst mode") ||

       !add_BuiltinCommand(sc, "enableAlphaCorrection(Boolean value)",
			   sc_enableAlphaCorrection_cmd, "Enables the Alpha/R-factor correction in the backend") ||

       !add_BuiltinCommand(sc, "setAlphaCorrection(Double value, [Channel channel, Stage stage])",
			   sc_setAlphaCorrection_cmd, "Sets the Alpha/R-factor correction coefficients") ||

       !add_BuiltinCommand(sc, "setNonlinCorrection(Double value, [Channel channel, Stage stage])",
			   sc_setNonlinCorrection_cmd, "Sets the Non-linearity correction coefficients")
       )

      return del_Script(sc);

  //------------------------------------------------------------
  // Add Roach command
  //------------------------------------------------------------
#if(1)
    if(!add_BuiltinCommand(sc, "generalRoachCommand(Command command, Double value, RoachNumber roachnumber)",
			   sc_generalRoachCommand_cmd, "Generalized command to the roach") )
      
      return del_Script(sc);
#endif  
  //------------------------------------------------------------
  // Add LNA Power Supply Commands
  //------------------------------------------------------------
  
    if(!add_BuiltinCommand(sc, "setDrainVoltage(Double value, [Lna lna, LnaStage stage])",
			   sc_setDrainVoltage_cmd, "Sets the Drain Voltage on the LNAs") ||
       
       !add_BuiltinCommand(sc, "setDrainCurrent(Double value, [Lna lna, LnaStage stage])",
			   sc_setDrainCurrent_cmd, "Sets the Drain Current on the LNAs") ||
       
       !add_BuiltinCommand(sc, "setGateVoltage(Double value, [Lna lna, LnaStage stage])",
			   sc_setGateVoltage_cmd, "Sets the Gate Voltage on the LNAs") ||

       !add_BuiltinCommand(sc, "setBias([Id current, Vd voltage, Lna lna, LnaStage stage])",
			   sc_setBias_cmd, "Sets the Bias on the LNAs") ||

       !add_BuiltinCommand(sc, "setModule([Lna lna, LnaStage stage])",
			   sc_setModule_cmd, "Sets the Bias on the LNAs") ||

       !add_BuiltinCommand(sc, "changeVoltage(Integer int1, Integer int2, Double value)", 
			   sc_changeVoltage_cmd, "Sets the Bias on the LNAs") ||

       !add_BuiltinCommand(sc, "getVoltage()",
			   sc_getVoltage_cmd, "Sets the Bias on the LNAs") ||

       !add_BuiltinCommand(sc, "enableBiasQuery(Boolean value)", 
			   sc_enableBiasQuery_cmd, "Sets the Bias on the LNAs"))

      return del_Script(sc);

  return sc;
}

/*.......................................................................
 * Implement a function that executes a specified program with the
 * given parameter string (if any)
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *
 * Input/Output:
 *  result     Variable *  The return value.
 *  state      Variable *  Unused.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_exec_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *command=0;   // The command string
  char *commandString=0; // Pointer to the string
  RtcNetCmd rtc;         // The network object to be sent to the
			 // real-time controller task

  // Get the command-line arguments.

  if(get_Arguments(args, &command, NULL))
    return 1;
   
  commandString = STRING_VARIABLE(command)->string;
  
  unsigned len = strlen(commandString);

  if(len > NET_LOG_MAX) {
    lprintf(stderr, "Command: %s exceeds the message buffer length\n",
	    commandString);
    return 1;
  }

  // Compose the real-time controller network command.

  rtc.antennas = cp_AntSet(cp)->getId();

  strncpy(rtc.cmd.runScript.script, commandString, len);
  rtc.cmd.runScript.script[len] = '\0';

  rtc.cmd.runScript.seq = schNextSeq(cp_Scheduler(cp), 
				     TransactionStatus::TRANS_SCRIPT);

  if(queue_rtc_command(cp, &rtc, NET_RUN_SCRIPT_CMD)) 
    return 1;

  return 0;
}

/**.......................................................................
 * Implement a function that returns true when a given set of operations
 * have completed.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        AcquireTargets - The set of targets to
 *                                         check.
 * Input/Output:
 *  result     Variable *  The return value.
 *  state      Variable *  Unused.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static FUNC_FN(sc_acquired_fn)
{
  ControlProg *cp = (ControlProg* )sc->project;  /* The
						    control-program
						    resource
						    container */
  Scheduler *sch = (Scheduler* )cp_Scheduler(cp); /* The resource
						     object of this
						     thread */
  Variable *vtarget;   /* The set of targets */
  Variable *vant;      /* The set of antenans */
  int completed = 1;   /* True if the specified operations have completed */
  unsigned set;        /* The set of operations to check. */
  unsigned antennas;

  // Get the command-line arguments.

  if(get_Arguments(args, &vtarget, &vant, NULL))
    return 1;

  set = SET_VARIABLE(vtarget)->set;

  antennas = OPTION_HAS_VALUE(vant) ? 
    SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();
  
  // Accumulate the combined completion status of all the specified
  // operations.

  // The following don't depend on antennas

  if(set & ACQ_MARK)
    completed = completed && schDone(sch, TransactionStatus::TRANS_MARK);
  if(set & ACQ_GRAB)
    completed = completed && schDone(sch, TransactionStatus::TRANS_GRAB);
  if(set & ACQ_SETREG)
    completed = completed && schDone(sch, TransactionStatus::TRANS_SETREG);
  if(set & ACQ_FRAME)
    completed = completed && schDone(sch, TransactionStatus::TRANS_FRAME);
  if(set & ACQ_SOURCE)
    completed = completed && schDone(sch, TransactionStatus::TRANS_PMAC);
  if(set & ACQ_SCAN)
    completed = completed && schDone(sch, TransactionStatus::TRANS_SCAN);
  if(set & ACQ_BENCH)
    completed = completed && schDone(sch, TransactionStatus::TRANS_BENCH);
  if(set & ACQ_SCRIPT) {
    completed = completed && schDone(sch, TransactionStatus::TRANS_SCRIPT);
  }
  
  // Record the result for return.

  BOOL_VARIABLE(result)->boolvar = completed;

  return 0;
}

/**.......................................................................
 * Implement a function that sets up the digital filter for DIO data
 */
static CMD_FN(sc_setFilter_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vfreq; // The cutoff frequency in Hz
  Variable *vtaps; // The number of taps
  OutputStream *output = sc->output;
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vfreq, &vtaps, NULL))
    return 1;
    
  // Compose the real-time controller network command.

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.setFilter.mask = FILTER_NONE;
  
  if(OPTION_HAS_VALUE(vfreq)) {
    rtc.cmd.setFilter.freqHz = DOUBLE_VARIABLE(vfreq)->d;
    rtc.cmd.setFilter.mask   = FILTER_FREQ;
  }

  if(OPTION_HAS_VALUE(vtaps)) {
    rtc.cmd.setFilter.ntaps = UINT_VARIABLE(vtaps)->uint;
    rtc.cmd.setFilter.mask = FILTER_NTAP;
  }
  
  // Send the command to the real-time controller.

  if(rtc.cmd.setFilter.mask != FILTER_NONE)
    if(queue_rtc_command(cp, &rtc, NET_SETFILTER_CMD)) {
      return 1;
    }

  return 0;    
}

/*.......................................................................
 * Tell the logger where to place subsequent log files.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         dir  -  The new log-file directory.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_scriptDir_cmd)
{
  RtcNetCmd rtc;                 /* The network object to be sent to the */

  ControlProg *cp = (ControlProg* )sc->project;  /* The context of the
						    host control
						    program */
  Variable *vdir;     /* The directory-name argument */
  LoggerMessage msg;  /* The message to be sent to the logger thread */
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vdir, NULL))
    return 1;
  
  // Send the request to the mediator

  char* dir = STRING_VARIABLE(vdir)->string;
  unsigned len = strlen(dir);

  if(len > NET_LOG_MAX) {
    lprintf(stderr, "Directory path: %s exceeds the message buffer length\n",
	    dir);
    return 1;
  }

  strcpy(rtc.cmd.scriptDir.dir, dir);

  if(queue_rtc_command(cp, &rtc, NET_SCRIPT_DIR_CMD))
    return 1;
  
  return 0;
}

//-----------------------------------------------------------------------
// Implement cryocon commands
//-----------------------------------------------------------------------

/**.......................................................................
 * Command to set up the Cryocon loop (downloads all default parameters)
 */
static CMD_FN(sc_setUpCryoLoop_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vLoopNum; // The loop number
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vLoopNum, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.gpib.cmdId      = gcp::control::GPIB_SETUP;
  rtc.cmd.gpib.intVals[0] = INT_VARIABLE(vLoopNum)->i;


  COUT("in specificscript.c");
  COUT("cmdId set to: " << rtc.cmd.gpib.cmdId);
  COUT("gcp::control::GPIB_SETUP: " << gcp::control::GPIB_SETUP);

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_GPIB_CMD))
    return 1;

  return 0;    
}


/**.......................................................................
 * Command to heat up the cryocon sensor
 */
static CMD_FN(sc_heatUpCryoSensor_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vLoopNum; // The loop number
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vLoopNum, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.gpib.cmdId      = gcp::control::GPIB_HEAT;
  rtc.cmd.gpib.intVals[0] = INT_VARIABLE(vLoopNum)->i;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_GPIB_CMD))
    return 1;

  return 0;    
}



/**.......................................................................
 * Command to resume cooling of sensor
 */
static CMD_FN(sc_resumeCryoCooling_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vLoopNum; // The loop number
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vLoopNum, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.gpib.cmdId      = gcp::control::GPIB_COOL;
  rtc.cmd.gpib.intVals[0] = INT_VARIABLE(vLoopNum)->i;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_GPIB_CMD))
    return 1;

  return 0;    
}


/**.......................................................................
 * Command to reset the module
 */
static CMD_FN(sc_resetCryoModule_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.gpib.cmdId      = gcp::control::GPIB_RESET;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_GPIB_CMD))
    return 1;

  return 0;    
}


/**.......................................................................
 * Command to reset the module
 */
static CMD_FN(sc_stopCryoControlLoop_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.gpib.cmdId      = gcp::control::GPIB_STOP_LOOP;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_GPIB_CMD))
    return 1;

  return 0;    
}


/**.......................................................................
 * Command to restart the loop
 */
static CMD_FN(sc_engageCryoControlLoop_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.gpib.cmdId      = gcp::control::GPIB_ENGAGE_LOOP;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_GPIB_CMD))
    return 1;

  return 0;    
}



/**.......................................................................
 * Command to set the Sky Temperature
 */
static CMD_FN(sc_setCryoSkyTemp_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vLoopNum; // The loop number
  Variable* vTemp;    // The output temperature desired
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vLoopNum, &vTemp, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.gpib.cmdId      = gcp::control::GPIB_SET_SKY_TEMP;
  rtc.cmd.gpib.intVals[0] = INT_VARIABLE(vLoopNum)->i;
  rtc.cmd.gpib.fltVal     = DOUBLE_VARIABLE(vTemp)->d;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_GPIB_CMD))
    return 1;

  return 0;    
}


/**.......................................................................
 * Command to set the source channel
 */
static CMD_FN(sc_setCryoSourceChannel_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vLoopNum; // The loop number
  Variable* vChannel; // The channel desired
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vLoopNum, &vChannel, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.gpib.cmdId      = gcp::control::GPIB_SET_CHANNEL;
  rtc.cmd.gpib.intVals[0] = INT_VARIABLE(vLoopNum)->i;
  rtc.cmd.gpib.intVals[1] = INT_VARIABLE(vChannel)->i;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_GPIB_CMD))
    return 1;

  return 0;    
}


/**.......................................................................
 * Command to set Loop Range (values of 1-3 only)
 */
static CMD_FN(sc_setCryoLoopRange_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vLoopNum; // The loop number
  Variable* vRange;   // The loop Range
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vLoopNum, &vRange, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.gpib.cmdId      = gcp::control::GPIB_SET_LOOP_RANGE;
  rtc.cmd.gpib.intVals[0] = INT_VARIABLE(vLoopNum)->i;
  rtc.cmd.gpib.intVals[1] = INT_VARIABLE(vRange)->i;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_GPIB_CMD))
    return 1;

  return 0;    
}


/**.......................................................................
 * Command to set the Proportional Gain in PID loop
 */
static CMD_FN(sc_setCryoPGain_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vLoopNum; // The loop number
  Variable* vGain;    // The gain value desired
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vLoopNum, &vGain, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.gpib.cmdId      = gcp::control::GPIB_SET_P;
  rtc.cmd.gpib.intVals[0] = INT_VARIABLE(vLoopNum)->i;
  rtc.cmd.gpib.fltVal     = DOUBLE_VARIABLE(vGain)->d;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_GPIB_CMD))
    return 1;

  return 0;    
}

/**.......................................................................
 * Command to set the Integral Gain in PID loop
 */
static CMD_FN(sc_setCryoIGain_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vLoopNum; // The loop number
  Variable* vGain;    // The gain value desired
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vLoopNum, &vGain, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.gpib.cmdId      = gcp::control::GPIB_SET_I;
  rtc.cmd.gpib.intVals[0] = INT_VARIABLE(vLoopNum)->i;
  rtc.cmd.gpib.fltVal     = DOUBLE_VARIABLE(vGain)->d;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_GPIB_CMD))
    return 1;

  return 0;    
}

/**.......................................................................
 * Command to set the Differential Gain in PID loop
 */
static CMD_FN(sc_setCryoDGain_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vLoopNum; // The loop number
  Variable* vGain;    // The gain value desired
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vLoopNum, &vGain, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.gpib.cmdId      = gcp::control::GPIB_SET_D;
  rtc.cmd.gpib.intVals[0] = INT_VARIABLE(vLoopNum)->i;
  rtc.cmd.gpib.fltVal     = DOUBLE_VARIABLE(vGain)->d;

  COUT("in specificscript.c");
  COUT("cmdId set to: " << rtc.cmd.gpib.cmdId);
  COUT("gcp::control::GPIB_SET_D: " << gcp::control::GPIB_SET_D);

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_GPIB_CMD))
    return 1;

  return 0;    
}


/**.......................................................................
 * Command to set the Cryocon power output
 */
static CMD_FN(sc_setCryoPowerOutput_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vLoopNum; // The loop number
  Variable* vPower;   // The output power level
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vLoopNum, &vPower, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.gpib.cmdId      = gcp::control::GPIB_SET_POWER_OUTPUT;
  rtc.cmd.gpib.intVals[0] = INT_VARIABLE(vLoopNum)->i;
  rtc.cmd.gpib.fltVal     = DOUBLE_VARIABLE(vPower)->d;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_GPIB_CMD))
    return 1;

  return 0;    
}

/**.......................................................................
 * Command to set the Cryocon heater load
 */
static CMD_FN(sc_setCryoHeaterLoad_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vLoopNum; // The loop number
  Variable* vLoad;    // The Heater load
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vLoopNum, &vLoad, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.gpib.cmdId      = gcp::control::GPIB_SET_HEATER_LOAD;
  rtc.cmd.gpib.intVals[0] = INT_VARIABLE(vLoopNum)->i;
  rtc.cmd.gpib.fltVal     = DOUBLE_VARIABLE(vLoad)->d;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_GPIB_CMD))
    return 1;

  return 0;    
}



/**.......................................................................
 * Command to set Control Type (values of 0-4 only)
 */
static CMD_FN(sc_setCryoControlType_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vLoopNum; // The loop number
  Variable* vType;    // The control loop type
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vLoopNum, &vType, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------
 
  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.gpib.cmdId      = gcp::control::GPIB_SET_LOOP_TYPE;
  rtc.cmd.gpib.intVals[0] = INT_VARIABLE(vLoopNum)->i;
  rtc.cmd.gpib.intVals[1] = INT_VARIABLE(vType)->i;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_GPIB_CMD))
    return 1;

  return 0;    
}


/**.......................................................................
 * Command to engage the servo on the antenna
 */
static CMD_FN(sc_engageServo_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  Variable* vstate;   // on/off
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vstate, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.servo.cmdId      = gcp::control::SERVO_ENGAGE;
  if(CHOICE_VARIABLE(vstate)->choice == SWITCH_ON){
    rtc.cmd.servo.intVal = 1;
  } else {
    rtc.cmd.servo.intVal = 0;
  }    

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_SERVO_CMD))
    return 1;
 
  return 0;    
}


/**.......................................................................
 * Command to enable the clutches on the antenna
 */
static CMD_FN(sc_enableClutches_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  Variable* vstate;   // on/off
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vstate, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.servo.cmdId      = gcp::control::SERVO_ENABLE_CLUTCHES;
  if(CHOICE_VARIABLE(vstate)->choice == SWITCH_ON){
    rtc.cmd.servo.intVal = 1;
  } else {
    rtc.cmd.servo.intVal = 0;
  }    

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_SERVO_CMD))
    return 1;
 
  return 0;    
}

/**.......................................................................
 * Command to enable the brakes on the antenna
 */
static CMD_FN(sc_enableBrakes_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  Variable* vstate;   // on/off
  Variable* axisVal;  // axis to apply the command to

  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vstate, &axisVal, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.servo.cmdId      = gcp::control::SERVO_ENABLE_BRAKES;
  if(CHOICE_VARIABLE(vstate)->choice == SWITCH_ON){
    rtc.cmd.servo.intVal = 1;
  } else {
    rtc.cmd.servo.intVal = 0;
  }    

  switch(CHOICE_VARIABLE(axisVal)->choice) {
  case AXIS_AZ:
    rtc.cmd.servo.fltVal = 1;
    break;

  case AXIS_EL:
    rtc.cmd.servo.fltVal = 2;
    break;

  case AXIS_ALL:
    rtc.cmd.servo.fltVal = 3;
    break;

  case AXIS_BOTH:
    rtc.cmd.servo.fltVal = 3;
    break;

  }


  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_SERVO_CMD))
    return 1;
 
  return 0;    
}

/**.......................................................................
 * Command to enable the contactors on the antenna
 */
static CMD_FN(sc_enableContactors_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  Variable* vstate;   // on/off
  Variable* axisVal;  // axis to apply the command to

  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vstate, &axisVal, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.servo.cmdId      = gcp::control::SERVO_ENABLE_CONTACTORS;
  if(CHOICE_VARIABLE(vstate)->choice == SWITCH_ON){
    rtc.cmd.servo.intVal = 1;
  } else {
    rtc.cmd.servo.intVal = 0;
  }    

  switch(CHOICE_VARIABLE(axisVal)->choice) {
  case AXIS_AZ:
    rtc.cmd.servo.fltVal = 1;
    break;

  case AXIS_EL:
    rtc.cmd.servo.fltVal = 2;
    break;

  case AXIS_ALL:
    rtc.cmd.servo.fltVal = 3;
    break;

  case AXIS_BOTH:
    rtc.cmd.servo.fltVal = 3;
    break;

  }

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_SERVO_CMD))
    return 1;
 
  return 0;    
}


/**.......................................................................
 * Command to initialize the antenna
 */
static CMD_FN(sc_servoInitializeAntenna_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.servo.cmdId      = gcp::control::SERVO_INITIALIZE_ANTENNA;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_SERVO_CMD))
    return 1;

  return 0;    
}


/**.......................................................................
 * Command to set maximum az velocity
 */
static CMD_FN(sc_setServoLoopParameters_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* loopNum;  // Loop index
  Variable* v1;       // loop parameter1
  Variable* v2;       // loop parameter2
  Variable* v3;       // loop parameter3
  Variable* v4;       // loop parameter4
  Variable* v5;       // loop parameter5
  Variable* v6;       // loop parameter6
  Variable* v7;       // loop parameter7
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &loopNum, &v1, &v2, &v3, &v4, &v5, &v6, &v7, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.servo.cmdId     = gcp::control::SERVO_LOAD_PARAMETERS;
  rtc.cmd.servo.intVal    = INT_VARIABLE(loopNum)->i;
  rtc.cmd.servo.fltVal    = 0.0;
  rtc.cmd.servo.fltVals[0]  = DOUBLE_VARIABLE(v1)->d;
  rtc.cmd.servo.fltVals[1]  = DOUBLE_VARIABLE(v2)->d;
  rtc.cmd.servo.fltVals[2]  = DOUBLE_VARIABLE(v3)->d;
  rtc.cmd.servo.fltVals[3]  = DOUBLE_VARIABLE(v4)->d;
  rtc.cmd.servo.fltVals[4]  = DOUBLE_VARIABLE(v5)->d;
  rtc.cmd.servo.fltVals[5]  = DOUBLE_VARIABLE(v6)->d;
  rtc.cmd.servo.fltVals[6]  = DOUBLE_VARIABLE(v7)->d;

  
  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_SERVO_CMD))
    return 1;

  return 0;    
}


/**.......................................................................
 * Command to setup the ADCs for getting data from the backend
 */
static CMD_FN(sc_setupAdc_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.rx.cmdId        = gcp::control::RX_SETUP_ADC;
  rtc.cmd.rx.fltVal       = 0;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
    return 1;

  return 0;    
}

/**.......................................................................
 * Command to reset the memory on the fpga
 */
static CMD_FN(sc_resetFpga_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.rx.cmdId        = gcp::control::RX_RESET_FPGA;
  rtc.cmd.rx.fltVal       = 0;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
    return 1;

  return 0;    
}

/**.......................................................................
 * Command to reset the memory on the backend fifo
 */
static CMD_FN(sc_resetFifo_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.rx.cmdId        = gcp::control::RX_RESET_FIFO;
  rtc.cmd.rx.fltVal       = 0;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
    return 1;

  return 0;    
}



/**.......................................................................
 * Command to set the burst length 
 */
static CMD_FN(sc_setBurstLength_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue;   // Length Value
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.rx.cmdId        = gcp::control::RX_SET_BURST_LENGTH;
  rtc.cmd.rx.fltVal       = DOUBLE_VARIABLE(vValue)->d;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
    return 1;

  return 0;    

}


/**.......................................................................
 * Command to set the Walshing period 
 */
static CMD_FN(sc_setSwitchPeriod_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue;   // Length Value
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.rx.cmdId        = gcp::control::RX_SET_SWITCH_PERIOD;
  rtc.cmd.rx.fltVal       = DOUBLE_VARIABLE(vValue)->d;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
    return 1;

  return 0;    

}

/**.......................................................................
 * Command to set the integration period
 */
static CMD_FN(sc_setIntegrationPeriod_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue;   // Length Value
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.rx.cmdId        = gcp::control::RX_SET_INTEGRATION_PERIOD;
  rtc.cmd.rx.fltVal       = DOUBLE_VARIABLE(vValue)->d;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
    return 1;

  return 0;    

}

/**.......................................................................
 * Command to set the number of samples to trim around the 1PPS
 */
static CMD_FN(sc_setTrimLength_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue;   // Length Value
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.rx.cmdId        = gcp::control::RX_SET_TRIM_LENGTH;
  rtc.cmd.rx.fltVal       = DOUBLE_VARIABLE(vValue)->d;

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
    return 1;

  return 0;    

}

/**.......................................................................
 * Enable the simulator mode in the backend.
 */
static CMD_FN(sc_enableRxSimulator_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue;   // True/False for enable/disable
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue, NULL))
    return 1;

  // Parse the command line arguments
  bool value = BOOL_VARIABLE(vValue)->boolvar;

  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.rx.cmdId        = gcp::control::RX_ENABLE_SIMULATOR;
  rtc.cmd.rx.fltVal       = double(value);

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
    return 1;

  return 0;    

}


/**.......................................................................
 * Enable the noise source in the backend
 */
static CMD_FN(sc_enableRxNoise_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue;   // True/False for enable/disable
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue, NULL))
    return 1;

  // Parse the command line arguments
  bool value = BOOL_VARIABLE(vValue)->boolvar;

  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.rx.cmdId        = gcp::control::RX_ENABLE_NOISE;
  rtc.cmd.rx.fltVal       = double(value);

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
    return 1;

  return 0;    

}


/**.......................................................................
 * Enable the walshing in the backend
 */
static CMD_FN(sc_enableWalshing_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue;   // True/False for enable/disable
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue, NULL))
    return 1;

  // Parse the command line arguments
  bool value = BOOL_VARIABLE(vValue)->boolvar;

  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.rx.cmdId        = gcp::control::RX_ENABLE_WALSHING;
  rtc.cmd.rx.fltVal       = double(value);

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
    return 1;

  return 0;    

}

/**.......................................................................
 * Enable the alternate walshing in the backend
 */
static CMD_FN(sc_enableAltWalshing_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue;   // True/False for enable/disable
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue, NULL))
    return 1;

  // Parse the command line arguments
  bool value = BOOL_VARIABLE(vValue)->boolvar;

  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.rx.cmdId        = gcp::control::RX_ENABLE_ALT_WALSHING;
  rtc.cmd.rx.fltVal       = double(value);

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  COUT("MAde it through enable alpha command");

  //  if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
    return 1;

  return 0;    

}


/**.......................................................................
 * Enable the noise source in the backend
 */
static CMD_FN(sc_enableAlphaCorrection_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue;   // True/False for enable/disable
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue, NULL))
    return 1;

  // Parse the command line arguments
  bool value = BOOL_VARIABLE(vValue)->boolvar;

  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.rx.cmdId        = gcp::control::RX_ENABLE_ALPHA;
  rtc.cmd.rx.fltVal       = double(value);

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
    return 1;

  return 0;    

}




/**.......................................................................
 * Enable the full walshing in the backend
 */
static CMD_FN(sc_enableFullWalshing_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue;   // True/False for enable/disable
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue, NULL))
    return 1;

  // Parse the command line arguments
  bool value = BOOL_VARIABLE(vValue)->boolvar;

  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.rx.cmdId        = gcp::control::RX_ENABLE_FULL_WALSHING;
  rtc.cmd.rx.fltVal       = double(value);

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
    return 1;

  return 0;    

}

/**.......................................................................
 * Enable the nonlinearity correction in the backend
 */
static CMD_FN(sc_enableNonLinearity_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue;   // True/False for enable/disable
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue, NULL))
    return 1;

  // Parse the command line arguments
  bool value = BOOL_VARIABLE(vValue)->boolvar;

  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.rx.cmdId        = gcp::control::RX_ENABLE_NONLINEARITY;
  rtc.cmd.rx.fltVal       = double(value);

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
    return 1;

  return 0;    

}

/**.......................................................................
 * Command to get a burst of data from the backend.
 */
static CMD_FN(sc_getBurstData_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, NULL))
    return 1;
    
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.rx.cmdId        = gcp::control::RX_GET_BURST_DATA;
  rtc.cmd.rx.fltVal       = 0;

  COUT("RX_GET_BURST_DATA: " << gcp::control::RX_GET_BURST_DATA);
  COUT("COMMAND ISSUED:  NET ID: " << rtc.cmd.rx.cmdId);

  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------

  if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
    return 1;

  return 0;    
};


/**.......................................................................
 * Command to set Alpha Correction Values
 */
static CMD_FN(sc_setAlphaCorrection_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue;       // value to set things to
  Variable* vStage;         // stage number
  Variable* vChannel;       // channel number
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task

  int channelVal;
  int stageVal;
  float value;
  bool doIt;
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue, &vChannel, &vStage, NULL))
    return 1;

  // read in arguments
  value = DOUBLE_VARIABLE(vValue)->d;

  
  // Check that the optional arguments have values
  if(OPTION_HAS_VALUE(vChannel)) {
    channelVal = INT_VARIABLE(vChannel)->i;
    if(channelVal > 2){
      lprintf(stderr, "Alpha Correction Channel values can only take values of 1 or 2.\n");
      doIt = false;
    } else {
      doIt = true;
    }
  }  else {
    lprintf(stderr, "No Channel number specified.  Will not execute command.\n");
    doIt = false;
  };

  if(OPTION_HAS_VALUE(vStage)) {
    stageVal = INT_VARIABLE(vStage)->i;
    doIt = true;
  }  else {
    doIt = false;
    lprintf(stderr, "No Stage Value specified. Will not execute command.\n");
  };
  
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)
  if(doIt){
    rtc.antennas = cp_AntSet(cp)->getId();
    
    
    rtc.cmd.rx.cmdId         = gcp::control::RX_SET_ALPHA;
    rtc.cmd.rx.stageNumber   = stageVal;
    rtc.cmd.rx.channelNumber = channelVal; 
    rtc.cmd.rx.fltVal        = value;
    COUT("sending message");

    //------------------------------------------------------------
    // Send the command to the real-time controller
    //------------------------------------------------------------

    
    if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
      return 1;
  }

  return 0;    
}



/**.......................................................................
 * Command to set Nonlinearity Correction Values
 */
static CMD_FN(sc_setNonlinCorrection_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue;       // value to set things to
  Variable* vStage;         // LNA number
  Variable* vChannel;       // LNA stage number
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task

  int channelVal;
  int stageVal;
  float value;
  bool doIt;
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue, &vChannel, &vStage, NULL))
    return 1;

  // read in arguments
  value = DOUBLE_VARIABLE(vValue)->d;
  COUT("setting to: " << value);

  
  // Check that the optional arguments have values
  if(OPTION_HAS_VALUE(vChannel)) {
    channelVal = INT_VARIABLE(vChannel)->i;
    doIt = true;
    COUT("Channel value:  " << channelVal);
  }  else {
    lprintf(stderr, "No Channel number specified.  Will not execute command.\n");
    doIt = false;
  };

  if(OPTION_HAS_VALUE(vStage)) {
    stageVal = INT_VARIABLE(vStage)->i;
    doIt = true;
    COUT("Stage value: " << stageVal);
  }  else {
    doIt = false;
    lprintf(stderr, "No Stage Value specified. Will not execute command.\n");
  };
  
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)
  if(doIt){
    rtc.antennas = cp_AntSet(cp)->getId();
    
    
    rtc.cmd.rx.cmdId         = gcp::control::RX_SET_NONLIN;
    rtc.cmd.rx.stageNumber   = stageVal;
    rtc.cmd.rx.channelNumber = channelVal; 
    rtc.cmd.rx.fltVal        = value;
    COUT("sending message");

    //------------------------------------------------------------
    // Send the command to the real-time controller
    //------------------------------------------------------------
    
    if(queue_rtc_command(cp, &rtc, NET_RX_CMD))
      return 1;
  }

  return 0;    
}

#if(1)
/**.......................................................................
 * General Roach Command
 */
static CMD_FN(sc_generalRoachCommand_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable *command=0;   // The command string
  char *commandString=0; // Pointer to the string
  Variable* vValue;       // value to set things to
  Variable* vRoach;       // Roach Number
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  
  int roachVal;
  float value;
  
  char* sendString[11];
  
  // Get the command-line arguments.
  
  if(get_Arguments(args, &command, &vValue, &vRoach, NULL)){
    return 1;
  }   
  commandString = STRING_VARIABLE(command)->string;
  unsigned len = strlen(commandString);
  COUT("COMMAND STRING: " << commandString);
  
  if(len>10){
    lprintf(stderr, "Command too long.  Cutting to 10 characters\n");
  }
  len = 10;

  // read in arguments
  value = DOUBLE_VARIABLE(vValue)->d;

  roachVal = INT_VARIABLE(vRoach)->i;
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)
  rtc.antennas = cp_AntSet(cp)->getId();
  


  rtc.cmd.roach.cmdId         = 1;
  strncpy(rtc.cmd.roach.stringCommand, commandString, len);
  rtc.cmd.roach.roachNum      = roachVal;
  rtc.cmd.roach.fltVal        = value;
  
  
  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------
  
  if(queue_rtc_command(cp, &rtc, NET_ROACH_CMD))
    return 1;

  return 0;    
}
#endif

/**.......................................................................
 * Command to set LNA Drain Voltage
 */
static CMD_FN(sc_setDrainVoltage_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue;       // value to set things to
  Variable* vLna;         // LNA number
  Variable* vStage;       // LNA stage number
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task

  int lnaVal;
  int stageVal;
  float value;
  bool doIt;
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue, &vLna, &vStage, NULL))
    return 1;

  // read in arguments
  value = DOUBLE_VARIABLE(vValue)->d;
  COUT("setting to: " << value);

  
  // Check that the optional arguments have values
  if(OPTION_HAS_VALUE(vLna)) {
    lnaVal = INT_VARIABLE(vLna)->i;
    doIt = true;
    COUT("LNA value:  " << lnaVal);
  }  else {
    lprintf(stderr, "No LNA number specified.  Will not execute command.\n");
    doIt = false;
  };

  if(OPTION_HAS_VALUE(vStage)) {
    stageVal = INT_VARIABLE(vStage)->i;
    doIt = true;
    COUT("LNA stage value: " << stageVal);
  }  else {
    doIt = false;
    lprintf(stderr, "No LNA number specified.  Will not execute command.\n");
  };
  
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)
  if(doIt){
    rtc.antennas = cp_AntSet(cp)->getId();
    
    
    rtc.cmd.lna.cmdId       = gcp::control::LNA_SET_DRAIN_VOLTAGE;
    rtc.cmd.lna.drainCurrent= 0;
    rtc.cmd.lna.drainVoltage= value;
    rtc.cmd.lna.lnaNumber   = lnaVal;
    rtc.cmd.lna.stageNumber = stageVal;

    COUT("sending message");

    //------------------------------------------------------------
    // Send the command to the real-time controller
    //------------------------------------------------------------
    
    if(queue_rtc_command(cp, &rtc, NET_LNA_CMD))
      return 1;
  }

  return 0;    
}


/**.......................................................................
 * Command to set LNA Drain Current
 */
static CMD_FN(sc_setDrainCurrent_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue;       // value to set things to
  Variable* vLna;         // LNA number
  Variable* vStage;       // LNA stage number
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task

  int lnaVal;
  int stageVal;
  float value;
  bool doIt;
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue, &vLna, &vStage, NULL))
    return 1;

  // read in arguments
  value = DOUBLE_VARIABLE(vValue)->d;
  COUT("setting to: " << value);

  
  // Check that the optional arguments have values
  if(OPTION_HAS_VALUE(vLna)) {
    lnaVal = INT_VARIABLE(vLna)->i;
    doIt = true;
    COUT("LNA value:  " << lnaVal);
  }  else {
    lprintf(stderr, "No LNA number specified.  Will not execute command.\n");
    doIt = false;
  };

  if(OPTION_HAS_VALUE(vStage)) {
    stageVal = INT_VARIABLE(vStage)->i;
    doIt = true;
    COUT("LNA stage value: " << stageVal);
  }  else {
    doIt = false;
    lprintf(stderr, "No LNA number specified.  Will not execute command.\n");
  };
  
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)
  if(doIt){
    rtc.antennas = cp_AntSet(cp)->getId();
    
    
    rtc.cmd.lna.cmdId       = gcp::control::LNA_SET_DRAIN_CURRENT;
    rtc.cmd.lna.drainVoltage = 0;
    rtc.cmd.lna.drainCurrent = value;
    rtc.cmd.lna.lnaNumber   = lnaVal;
    rtc.cmd.lna.stageNumber = stageVal;

    COUT("sending message");

    //------------------------------------------------------------
    // Send the command to the real-time controller
    //------------------------------------------------------------
    
    if(queue_rtc_command(cp, &rtc, NET_LNA_CMD))
      return 1;
  }

  return 0;    
}


/**.......................................................................
 * Command to set LNA Gate Voltage
 */
static CMD_FN(sc_setGateVoltage_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue;       // value to set things to
  Variable* vLna;         // LNA number
  Variable* vStage;       // LNA stage number
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task

  int lnaVal;
  int stageVal;
  float value;
  bool doIt;
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue, &vLna, &vStage, NULL))
    return 1;

  // read in arguments
  value = DOUBLE_VARIABLE(vValue)->d;
  COUT("setting to: " << value);

  
  // Check that the optional arguments have values
  if(OPTION_HAS_VALUE(vLna)) {
    lnaVal = INT_VARIABLE(vLna)->i;
    doIt = true;
    COUT("LNA value:  " << lnaVal);
  }  else {
    lprintf(stderr, "No LNA number specified.  Will not execute command.\n");
    doIt = false;
  };

  if(OPTION_HAS_VALUE(vStage)) {
    stageVal = INT_VARIABLE(vStage)->i;
    doIt = true;
    COUT("LNA stage value: " << stageVal);
  }  else {
    doIt = false;
    lprintf(stderr, "No LNA number specified.  Will not execute command.\n");
  };
  
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)
  if(doIt){
    rtc.antennas = cp_AntSet(cp)->getId();
    
    
    rtc.cmd.lna.cmdId       = gcp::control::LNA_SET_GATE_VOLTAGE;
    rtc.cmd.lna.drainVoltage = value;
    rtc.cmd.lna.drainCurrent = 0;
    rtc.cmd.lna.lnaNumber   = lnaVal;
    rtc.cmd.lna.stageNumber = stageVal;

    COUT("sending message: cmdId = " << rtc.cmd.lna.cmdId);

    //------------------------------------------------------------
    // Send the command to the real-time controller
    //------------------------------------------------------------
    
    if(queue_rtc_command(cp, &rtc, NET_LNA_CMD))
      return 1;
  }

  return 0;    
}


/**.......................................................................
 * Command to set a bias
 */
static CMD_FN(sc_setBias_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vDrainVoltage;       // value to set things to
  Variable* vDrainCurrent;       // value to set things to
  Variable* vLna;         // LNA number
  Variable* vStage;       // LNA stage number
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task

  int lnaVal;
  int stageVal;
  float drainCurrent, drainVoltage;
  bool doIt = true;
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vDrainCurrent, &vDrainVoltage, &vLna, &vStage, NULL))
    return 1;

  // read in arguments
  // Check that the optional arguments have values
  if(OPTION_HAS_VALUE(vDrainVoltage)) {
    drainVoltage = DOUBLE_VARIABLE(vDrainVoltage)->d;
  }  else {
    lprintf(stderr, "No Drain Voltage specified.  Will not execute command.\n");
    doIt = false;
  };

  if(OPTION_HAS_VALUE(vDrainCurrent)) {
    drainCurrent = DOUBLE_VARIABLE(vDrainCurrent)->d;
  }  else {
    lprintf(stderr, "No Drain Current specified.  Will not execute command.\n");
    doIt = false;
  };

  if(OPTION_HAS_VALUE(vLna)) {
    lnaVal = INT_VARIABLE(vLna)->i;
    COUT("LNA value:  " << lnaVal);
  }  else {
    lprintf(stderr, "No LNA number specified.  Will not execute command.\n");
    doIt = false;
  };

  if(OPTION_HAS_VALUE(vStage)) {
    stageVal = INT_VARIABLE(vStage)->i;
    COUT("LNA stage value: " << stageVal);
  }  else {
    doIt = false;
    lprintf(stderr, "No LNA number specified.  Will not execute command.\n");
  };
  
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)
  if(doIt){
    rtc.antennas = cp_AntSet(cp)->getId();
    
    
    rtc.cmd.lna.cmdId       = gcp::control::LNA_SET_BIAS;
    rtc.cmd.lna.drainVoltage = drainVoltage;
    rtc.cmd.lna.drainCurrent = drainCurrent;
    rtc.cmd.lna.lnaNumber   = lnaVal;
    rtc.cmd.lna.stageNumber = stageVal;

    COUT("sending message: cmdId = " << rtc.cmd.lna.cmdId);

    //------------------------------------------------------------
    // Send the command to the real-time controller
    //------------------------------------------------------------
    
    if(queue_rtc_command(cp, &rtc, NET_LNA_CMD))
      return 1;
  }

  return 0;    
}


/**.......................................................................
 * Command to set a module
 */
static CMD_FN(sc_setModule_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vLna;         // LNA number
  Variable* vStage;       // LNA stage number
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task

  int lnaVal;
  int stageVal;
  bool doIt = true;
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vLna, &vStage, NULL))
    return 1;

  // read in arguments
  // Check that the optional arguments have values
  if(OPTION_HAS_VALUE(vLna)) {
    lnaVal = INT_VARIABLE(vLna)->i;
    COUT("LNA value:  " << lnaVal);
  }  else {
    lprintf(stderr, "No LNA number specified.  Will not execute command.\n");
    doIt = false;
  };

  if(OPTION_HAS_VALUE(vStage)) {
    stageVal = INT_VARIABLE(vStage)->i;
    COUT("LNA stage value: " << stageVal);
  }  else {
    doIt = false;
    lprintf(stderr, "No LNA number specified.  Will not execute command.\n");
  };
  
  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)
  if(doIt){
    rtc.antennas = cp_AntSet(cp)->getId();
    
    
    rtc.cmd.lna.cmdId       = gcp::control::LNA_SET_MODULE;
    rtc.cmd.lna.lnaNumber   = lnaVal;
    rtc.cmd.lna.stageNumber = stageVal;

    COUT("sending message: cmdId = " << rtc.cmd.lna.cmdId);

    //------------------------------------------------------------
    // Send the command to the real-time controller
    //------------------------------------------------------------
    
    if(queue_rtc_command(cp, &rtc, NET_LNA_CMD))
      return 1;
  }

  return 0;    
}


/**.......................................................................
 * Command to change and LNA voltage
 */
static CMD_FN(sc_changeVoltage_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  Variable* vValue1;       // value to set things to
  Variable* vValue2;       // value to set things to
  Variable* vValue3;       // value to set things to
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task

  int upordown;
  int drainorgate;
  float value;
  bool doIt;
  
  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, &vValue1, &vValue2, &vValue3, NULL))
    return 1;

  // read in arguments
  upordown    = INT_VARIABLE(vValue1)->i;
  drainorgate = INT_VARIABLE(vValue2)->i;
  value       = DOUBLE_VARIABLE(vValue3)->d;

  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)
  rtc.antennas = cp_AntSet(cp)->getId();
  
  // this is ugly, but i don't feel like redefining the structure, so
  // as long as i'm consistent in the packing and executing commands,
  // we're ok.
  rtc.cmd.lna.cmdId       = gcp::control::LNA_CHANGE_VOLTAGE;
  rtc.cmd.lna.drainVoltage = value;
  rtc.cmd.lna.drainCurrent = 0;
  rtc.cmd.lna.lnaNumber   = upordown;
  rtc.cmd.lna.stageNumber = drainorgate;
  
  COUT("sending message: cmdId = " << rtc.cmd.lna.cmdId);
  
  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------
  
  if(queue_rtc_command(cp, &rtc, NET_LNA_CMD))
    return 1;
  
  return 0;    
}


/**.......................................................................
 * Command to get the LNA voltage
 */
static CMD_FN(sc_getVoltage_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  OutputStream *output = sc->output;
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task

  //------------------------------------------------------------ 
  // Get the command-line arguments.  Last argument must always be
  // NULL
  //------------------------------------------------------------

  if(get_Arguments(args, NULL))
    return 1;

  //------------------------------------------------------------
  // Compose the real-time controller network command.
  //------------------------------------------------------------

  // Default to all antennas (even if you only have one)
  rtc.antennas = cp_AntSet(cp)->getId();
  
  // this is ugly, but i don't feel like redefining the structure, so
  // as long as i'm consistent in the packing and executing commands,
  // we're ok.
  rtc.cmd.lna.cmdId       = gcp::control::LNA_GET_VOLTAGE;
  
  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------
  
  if(queue_rtc_command(cp, &rtc, NET_LNA_CMD))
    return 1;
  
  return 0;    
}


/**.......................................................................
 * Command to enable the bias query
 */
static CMD_FN(sc_enableBiasQuery_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object

  OutputStream *output = sc->output;
  RtcNetCmd rtc;      // The network object to be sent to the
		      // real-time controller task
  Variable* vValue; 

  // get arguments
  if(get_Arguments(args, &vValue, NULL))
    return 1;

  // Parse the command line arguments
  bool value = BOOL_VARIABLE(vValue)->boolvar;

  // Default to all antennas (even if you only have one)
  rtc.antennas = cp_AntSet(cp)->getId();

  // this is ugly, but i don't feel like redefining the structure, so
  // as long as i'm consistent in the packing and executing commands,
  // we're ok.
  rtc.cmd.lna.cmdId        = gcp::control::LNA_ENABLE_BIAS_QUERY;
  rtc.cmd.lna.stageNumber  = (int) value;
  
  //------------------------------------------------------------
  // Send the command to the real-time controller
  //------------------------------------------------------------
  
  if(queue_rtc_command(cp, &rtc, NET_LNA_CMD))
    return 1;
  
  return 0;    
}
