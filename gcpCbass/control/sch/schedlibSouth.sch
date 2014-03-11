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
#   21-JAN-2013:  MP add 'until ($acquired(source) | $elapsed > 3m)' in various places as $acquired 
#                    doesn't seem to be working poperly
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
#  servoInitializeAntenna
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
# Does a pointing cross on a source in distinct 0.125 deg steps.
#======================================================================

command do_point_cross(Source src){

  # tag the log file and mark the "pointing_cross" bit
  log "Starting command do_point_cross: ", $date
  offset az=0, el=0
  sky_offset x=0, y=0

  # track the source
  log "do_point_cross: starting to track source: ", $date
  track $src
  # until $acquired(source)
  until ($acquired(source) | $elapsed > 3m)
  mark add, f6

  log "do_point_cross: starting to do a pointing offset (y): ", $date
  do PointingOffset yoff= -0.75,0.751,0.125 {
	sky_offset y=$yoff
	until $acquired(source)
	mark add, f0
	until $elapsed > 10s
	mark remove, f0
  }
  log "do_point_cross: zeroing offset (y): ", $date
  sky_offset y=0

  log "do_point_cross: starting to do a pointing offset (x): ", $date
  do PointingOffset xoff= -0.75,0.751,0.125 {
	sky_offset x=$xoff
	until $acquired(source)
	mark add, f0
	until $elapsed > 10s
	mark remove, f0
  }
  log "do_point_cross: zeroing offset (x): ", $date
  mark remove, f6
  sky_offset x=0

  log "Exiting command do_point_cross: ", $date
}

#======================================================================
# Does a pointing cross on a source in distinct 0.5 deg steps.
#======================================================================

command do_five_point_cross(Source src)
{
  # tag the log file and mark the "pointing_cross" bit
  log "Starting command do_five_point_cross: ", $date
  offset az=0, el=0

  # track the source
  track $src
#  until $acquired(source)
  until ($acquired(source) | $elapsed > 3m)
  mark add, f6

  do PointingOffset eloff= -0.5,0.51,0.5 {
	offset el=$eloff
	until $acquired(source)
	mark add, f0
	until $elapsed > 1m
	mark remove, f0
  }
  offset el=0

  do PointingOffset azoff= -0.5,0.51,0.5 {
	offset az=$azoff
	until $acquired(source)
	mark add, f0
	until $elapsed > 1m
	mark remove, f0
  }
  offset az=0
  mark remove, f6

  log "Exiting command do_five_point_cross: ", $date
}


#======================================================================
# Does a pointing cross on a source in distinct 0.25 deg steps.
#======================================================================

command do_nine_point_cross(Source src)
{
  # tag the log file and mark the "pointing_cross" bit
  log "Starting command do_nine_point_cross: ", $date
  offset az=0, el=0

  # track the source
  track $src
#  until $acquired(source)
  until ($acquired(source) | $elapsed > 3m)
  mark add, f6

  do PointingOffset yoff= -0.5,0.51,0.25 {
	sky_offset y=$yoff
	until $acquired(source)
	mark add, f0
	until $elapsed > 1m
	mark remove, f0
  }
  sky_offset y=0

  do PointingOffset xoff= -0.5,0.51,0.25 {
	sky_offset x=$xoff
	until $acquired(source)
	mark add, f0
	until $elapsed > 1m
	mark remove, f0
  }
  sky_offset x=0
  mark remove, f6

  log "Exiting command do_nine_point_cross: ", $date
}

#======================================================================
# Does a pointing cross on a source in scan going 1degs/s 
#======================================================================

command do_point_scan(Source src)
{
  # tag the log file and mark the "pointing_scan" bit
  log "Starting command do_point_scan: ", $date

  log "do_point_scan: track source: ", $date
  offset az=0, el=0
  track $src
#  until $acquired(source)
  until ($acquired(source) | $elapsed > 1m)

  log "do_point_scan: mark add: ", $date
  # add the bit indicating we have good data:
  mark add, f0+f7
  until $acquired(mark)
  
  # do the scan 

  log "do_point_scan: scan source: ", $date
  scan point_cross_10daz_10del_0.5ds

  # wait for scan to end then remove the mark
  until ( $acquired(scan) | $elapsed>110s )
	
  log "do_point_scan: mark remove: ", $date
  mark remove, f0+f7
  until $acquired(mark)

  log "do_point_scan: zero offsets: ", $date
  zeroScanOffsets
  log "do_point_scan: track source: ", $date
  track $src
#  until $acquired(source)
  until ($acquired(source) | $elapsed > 1m)

  log "Exiting command do_point_scan: ", $date
}

command do_point_scan_large(Source src)
{
  # tag the log file and mark the "pointing_scan" bit
  log "Starting command do_point_scan_large: ", $date

  offset az=0, el=0
  track $src
#  until $acquired(source)
  until ($acquired(source) | $elapsed > 3m)

  # add the bit indicating we have good data:
  mark add, f0+f7
  until $acquired(mark)
  
  # do the scan 
  # NOTE: LOOK IN ~CBASS/GCPCBASS/CONTROL/SCAN/CBASS_SCAN.CAT FOR DEFINITIONS!
  scan point_cross_20daz_20del_1ds

  # wait for scan to end then remove the mark
  until $acquired(scan)
	
  mark remove, f0+f7
  until $acquired(mark)

  zeroScanOffsets
  track $src
  until $acquired(source)

  log "Exiting command do_point_scan_large: ", $date
}

#======================================================================
# Does a pointing scan on a source according to scanfile name
#======================================================================

command do_point_scan_gen(Source src, Scan scanName)
{
  # tag the log file and mark the "pointing_scan" bit
  log "Starting command do_point_scan_gen: ", $date
  mark add, f7

  offset az=0, el=0
  track $src
#  until $acquired(source)
  until ($acquired(source) | $elapsed > 3m)

  # add the bit indicating we have good data:
  mark add, f0
  until $acquired(mark)
  
  # do the scan 
  scan $scanName
  until $acquired(scan)

  # wait for scan to end then remove the mark
  until $acquired(scan)
	
  mark remove, f0+f7
  until $acquired(mark)

  log "Exiting command do_point_scan_gen: ", $date
}

#======================================================================
#Does a pointing scan on a source according to scanfile name.

#  This command is kind of silly when you can just put in a scan file that
#does this automatically.
#======================================================================

command do_point_raster(Source src, PointingOffset maxOff, Sexagesimal del, Scan scanName)
{
  # tag the log file and mark the "pointing_scan" bit
  log "Starting command do_point_raster: ", $date

  # calculate the offset ranges
  PointingOffset dStart = -$maxOff - $del
  PointingOffset dStop  = $maxOff

  offset az=0, el=0
  track $src
#  until $acquired(source)
  until ($acquired(source) | $elapsed > 3m)
  mark add, f7

  # add the bit indicating we have good data:
  do PointingOffset deltaEl = $dStart, $dStop, $del {
    
     offset az=0, el=$deltaEl
     until $acquired(source)
     mark add, f0
     until $acquired(mark)

     # do the scan 
     scan $scanName
     until $acquired(scan)

     mark remove, f0
     until $acquired(mark)
   }

  mark remove, f0+f7
  until $acquired(mark)

  offset az=0, el=0
  zeroScanOffsets

  log "Exiting command do_point_raster: ", $date
}



#======================================================================
#A command to do a sky dip
#======================================================================
command do_sky_dip(){

# first we slew to top
slew el=74
until $acquired(source) | $elapsed > 60s
until $elapsed > 3s

# next add the feature and do the dip
mark add, f5
scan el-0.5ds-70d-down-scan
until $elapsed > 150s
mark remove, f5

# zero offsets
zeroScanOffsets

until $elapsed>90s
}

#======================================================================
#A command to do a 50 degree to 30 degree sky dip from the current az
#======================================================================
command do_large_sky_dip(){
# Slew to 50 degree elevation
slew el=50
until $acquired(source)|$elapsed>2m
until $elapsed > 3s

# Add feature and do sky dip
mark add, f5
scan el-0.5ds-20d-down-scan
until $acquired(scan)|$elapsed>2m
mark remove, f5

zeroScanOffsets

}

#======================================================================
#A command to do a small sky dip from the current location
#======================================================================
command do_small_sky_dip(){

# next add the feature and do the dip
mark add, f5
scan el-0.5ds-10d-down-scan
until $acquired(scan)
mark remove, f5

# zero offsets
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

}
#======================================================================
#A command to turn the noise injection source on, take some data, and
#turn it off.  Spend some more time with it off.
#======================================================================
command noise_on_source_off(Interval time, Source src, Features feat, PointingOffset azoff)
{


# track the source
track $src
until $acquired(source)| $elapsed>1m
until $elapsed>3s
mark add, f12
mark add, $feat
noise_on($time)
mark remove, $feat

# slew off the source
offset az=$azoff
until $acquired(source)  | $elapsed>1m
until $elapsed>3s
mark add, f0
noise_on($time)
mark remove, f0

mark remove, f12

offset az=0
until $acquired(source)| $elapsed>1m
until $elapsed>1s

}




#======================================================================
#A command to turn the noise injection source on, take some data, and
#turn it off.  Spend some more time with it off.
#======================================================================
command noise_on_source(Interval time, Source src, Features feat)
{


# track the source
track $src
until $acquired(source)
until $elapsed>3s
mark add, f12
mark add, $feat
noise_on($time)
mark remove, $feat

# slew off the source
offset az=5
until $acquired(source)
until $elapsed>3s
mark add, f0
noise_on($time)
mark remove, f0

mark remove, f12

offset az=0
until $acquired(source)
until $elapsed>1s

}

#======================================================================
#A command to turn the noise injection source on, take some data, and
#turn it off.  This version allows you to set the off and on times seperately
#======================================================================
command noise_on_general(Interval offtime, Interval ontime)
{
# some time with no noise source
generalRoachCommand NDIODEXXXX,0,0
mark add, f11
until $elapsed>$offtime
mark remove, f11
#until $acquired(mark)

# With the noise source on.
generalRoachCommand NDIODEXXXX,1,0
#nableRxNoise true
mark add, f10
until $elapsed>$ontime
mark remove, f10
#until $acquired(mark)

# Again with the noise source off.
generalRoachCommand NDIODEXXXX,0,0
#enableRxNoise false
mark add, f11
until $elapsed>$offtime
mark remove, f11
#until $acquired(mark)

until $elapsed>1s

}

#======================================================================
# Does a pointing cross on a source in scan going 1degs/s 
#  allowing the scan to be centred about some offset position
#======================================================================

command do_point_scan_with_offsets(Source src, PointingOffset azoff, PointingOffset eloff)
{
  # tag the log file and mark the "pointing_scan" bit
  log "Starting command do_point_scan: ", $date

  log "do_point_scan: track source: ", $date
  offset az=$azoff, el=$eloff
  track $src
#  until $acquired(source)
  until ($acquired(source) | $elapsed > 1m)

  log "do_point_scan: mark add: ", $date
  # add the bit indicating we have good data:
  mark add, f0+f7
  until $acquired(mark)
  
  # do the scan 

  log "do_point_scan: scan source: ", $date
  scan point_cross_10daz_10del_0.5ds

  # wait for scan to end then remove the mark
  until ( $acquired(scan) | $elapsed>110s )
	
  log "do_point_scan: mark remove: ", $date
  mark remove, f0+f7
  until $acquired(mark)

  log "do_point_scan: zero offsets: ", $date
  zeroScanOffsets
  log "do_point_scan: track source: ", $date
  track $src
#  until $acquired(source)
  until ($acquired(source) | $elapsed > 1m)

  log "Exiting command do_point_scan: ", $date
}

#======================================================================
# Does a pointing cross on a source in user-defined steps.
#======================================================================

command do_point_cross_general(Source src, PointingOffset theta, PointingOffset dtheta, Interval dwell, PointingOffset xoff, PointingOffset yoff)
{

  # tag the log file and mark the "pointing_cross" bit
  log "Starting command do_point_cross_general: ", $date
  offset az=0, el=0
  sky_offset x=0, y=0

  # track the source
  log "do_point_cross: starting to track source: ", $date
  track $src
  # until $acquired(source)
  until ($acquired(source) | $elapsed > 1m)
  mark add, f6

  offset az=$xoff
  PointingOffset xymin = -$theta
  PointingOffset xymax = $theta+.001
  log "do_point_cross: starting to do a pointing offset (y): ", $date
  do PointingOffset yoff= $xymin,$xymax,$dtheta {
	#sky_offset y=$yoff
        offset el=$yoff
	until ($acquired(source)|$elapsed > 3s)
	mark add, f0
	until $elapsed > $dwell
	mark remove, f0
  }
  log "do_point_cross_general: zeroing offset (y): ", $date
  #sky_offset y=0
  offset el=$yoff

  log "do_point_cross_general: starting to do a pointing offset (x): ", $date
  do PointingOffset xoff= $xymin,$xymax,$dtheta {
	#sky_offset x=$xoff
	offset az=$xoff
	until ($acquired(source)|$elapsed > 3s)
	mark add, f0
	until $elapsed > $dwell
	mark remove, f0
  }
  log "do_point_cross_general: zeroing offset (x): ", $date
  mark remove, f6
  #sky_offset x=0
  offset az=0, el=0

  log "Exiting command do_point_cross: ", $date
}
