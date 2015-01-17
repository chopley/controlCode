# cycles through the list and observes sources from the list until the time is up.

import ~cbassuser/cbass/gcpCbass/control/sch/schedlibSouth.sch


scan_catalog ~cbassuser/cbass/gcpCbass/control/scan/cbass_scan.cat
# Define source list, with an lst for each one.  This list is
# created by rad_set.m in order to maximize sky coverage.

group Source {
  Source name,    # source name
  Features feat    # feature to add to source
}

#marking the features as follows, f1 is primary, f2 is seciondary
listof Source sources = {
{ (CygA)	,f1},
{ (Sgra)	,f1},
{ (Taua)   	,f1},
{ (M42)         ,f2},
{ (Sun)         ,f2}}

# start observation 
init_obs("Beginning schedule: scan_calibrators_2013South.sch")

# Set archiving to the correct mode
mark remove, all

slew az=0,el=30
until $acquired(source)|$elapsed>3m
slew az=-100
until $acquired(source)|$elapsed>1m

# start by taking a blank stare
track scpfixed
until $acquired(source)|$elapsed>1m
# Noise!
noise_on_general 5s, 8s

#-----------------------------------------------------------------------
# Main loop -- track the next source, make a scan, move on
#-----------------------------------------------------------------------

# Go through list
foreach(Source source) $sources {

  if $elevation($source.name) > 30 & $elevation($source.name) < 83 {

	# STEPHEN'S CALIBRATION SCHEME
	#  slew to source, do a pointing cross
	#      	measure its flux with the noise source on and off
	#  slew off source in azimuth, do a skydip


	# start by slewing to source and doing a pointing cross
	zeroScanOffsets
    	track $source.name
    	until $acquired(source)|$elapsed>3m
	mark add, f1
	
	# measure the source flux with the noise source on and off
	# We need noise diode events before and after the scans in
        # order to interpolate the alpha corrections
        log "starting the noise on/off routine" 

	if($elevation($source.name) > 75) {
            noise_on_source_off 10s, $source.name, f1, 30
	} else if($elevation($source.name) > 70) {
	    noise_on_source_off 10s, $source.name,  f1, 20
	} else { 
	  noise_on_source_off 10s,  $source.name,  f1, 15
        }

        log "done with noise on/off routine" 
        log "starting pointing routine" 
	# do two pointing crosses
	do_point_scan($source.name)
        log "done with  pointing routine" 
#	if($elevation($source.name) > 70) {
#	  noise_on_source_off 10s,  $source.name,  f1, 20
#	} else { 
#	  noise_on_source_off 10s,  $source.name,  f1, 15
#        }

        log "starting skydip routine"
	# offset from the source, and do a sky dip
	offset az=10
	until $acquired(source)|$elapsed>2m
	do_large_sky_dip
        log "done with skydip routine"

        log "resetting observation"
	# remove all offsets, track the source again
	offset az=0, el=0

#	track $source.name
#	until $acquired(source)

	# we're done:  remove our source feature
	mark remove, f1
 	log "next source"

    }
}
terminate("Ending schedule: scan_calibrators_2013.sch")
