######################################################
#-----------------------------------------------------
#   CBASS Schedule Library
#
#   last edited by OGK on Mar 4, 2011
#
#
#------------------------------------------------------
#         * * * IMPORTANT * * *
#------------------------------------------------------
#
# 	If you modify this library note the change below!
# 	If you add a command, add your initials and the date 
#	which it was added.
#
# 	Most schedules should be just a few calls against this library.
#
# 	You can gain access to the commands defined in this file by including 
# 	the line:
#
# 		import ~cbass/gcpCbass/control/sch/schedlib.sch
#
#------------------------------------------------------
# 	Features bits:
#------------------------------------------------------
#
#	f0 = analyze this data  (scan data/off source data)
# 	f1 = scan on source
# 	f2 = calibration source
#	f3 = 
#	f4 = blank sky (to find mains)
#	f5 = sky dip
#	f6 = radio pointing cross (distinct steps)
#	f7 = radio pointing scan  (two scans)
#	f8 = optical pointing
# 	f9 = beam map
#	f10 = noise injection on 
#       f11 = noise injection off 
#       f12 = noise_on_source
#       f13-f15 free
#	
#       f16 = ncp azimuth scans (for NCP survey)
#
#	f17-19 free
#
#------------------------------------------------------
# 	Contents of the library:
#------------------------------------------------------
#
# 	init_obs - very basic init script to remove features bits and offsets
#
# 	terminate - very basic termination script to remove offsets and features bits
#
# 	do_point_cross - doess a pointing cross on a source (distinct steps)
#
# 	do_five_point_cross - doess a five-point cross on a source -0.5 to 0.5 (distinct steps)
#
# 	do_nine_point_cross - doess a nine-point cross on a source -0.5 to 0.5 (distinct steps)
#
#
# 	do_point_scan_gen - does a scan in az about center, then a scan in el, according to whatever scan file you put as input.  also does rasters if that's what your scan file wants.
#
# 	do_point_scan - does a 10 deg scan in az about center, then a 10 deg scan in el (at 1 deg/s)
#
# 	do_point_scan_large - does a 50 deg scan in az about center, then a 10 deg scan in el (at 1 deg/s)
#
# 	do_point_raster - scans in az, steps in el.
#
#	do_sky_dip - does a 70 degree sky dip
#
#	do_small_sky_dip -- does a 10 degree sky dip
#
#	do_large_sky_dip -- does a 20 degree sky dip from 50 degrees to 30 degrees
#
#	noise_on - turns the noise injection on
#
#	noise_on_source - noise injection scheme where we turn it off and on while on and off a source
#
#	noise_on_general - turn the noise injection on, with options for different off and on times
#
#----->REMOVED:	restore_nominal_biases - sets the LNA biases to the nominal values
#	Import biasCombinations.sch and use the functions in there to modify the biases.
#
#------------------------------------------------------
#	HISTORY
#------------------------------------------------------
#   27-JAN-2010:  Created by SJCM
#   05-MAY-2010:  MAS added Feature bit f16 for NCP survey.
#   18-NOV-2010:  OGK added the restore_nominal_biases function.
#   02-MAR-2011:  OGK moved restore_nominal_biases function to biasCombinations.sch
#------------------------------------------------------
#######################################################


#======================================================================
#A command which should be run at the beginning of every schedule -
#very basic init script to remove features bits and offsets
#======================================================================

command init_obs(String logline) {

  # log a user-defined message:
  log $logline

  # remove all marks
  mark remove, all

  # remove any offsets:
  offset az=0, el=0
  sky_offset x=0, y=0
  radec_offset ra=0, dec=0
  zeroScanOffsets

  # recalibrate the encoders
  servoInitializeAntenna

}



#======================================================================
#A command which should be run at the end of every schedule - very
#basic termination script to remove offsets and features bits
#======================================================================

command terminate(String logline){

  # log a user-defined message:

  log $logline

  # remove all marks

  mark remove, all

  # remove any offsets:

  offset az=0, el=0
  sky_offset x=0, y=0
  radec_offset ra=0, dec=0
  zeroScanOffsets
}





#======================================================================
#A command to turn the noise injection source on, take some data, and
#turn it off.  Spend some more time with it off.
#======================================================================
command noise_on(Interval time){

# some time with no noise source
generalRoachCommand NDIODEXXXX,0,0
mark add, f11
until $elapsed>$time
mark remove, f11
until $acquired(mark)

# With the noise source on.
generalRoachCommand NDIODEXXXX,1,0
mark add, f10
until $elapsed>$time
mark remove, f10
until $acquired(mark)

# Again with the noise source off.
generalRoachCommand NDIODEXXXX,0,0
mark add, f11
until $elapsed>$time
mark remove, f11
until $acquired(mark)

until $elapsed>1s

#======================================================================
#A command to turn the noise injection source on, take some data, and
#turn it off.  This version allows you to set the off and on times seperately
#======================================================================
command noise_on_general(Interval offtime, Interval ontime)
{
# some time with no noise source
enableRxNoise false
mark add, f11
until $elapsed>$offtime
mark remove, f11
#until $acquired(mark)

# With the noise source on.
enableRxNoise true
mark add, f10
until $elapsed>$ontime
mark remove, f10
#until $acquired(mark)

# Again with the noise source off.
enableRxNoise false
mark add, f11
until $elapsed>$offtime
mark remove, f11
#until $acquired(mark)

until $elapsed>1s

}

