(Time utcstart, Time utcstop, Source source)
# Do pointing crosses on selected source until utc stop
# Currently defined start and stop time in utc for ease of scheduling real time
# - you need to check when the source is going to b up on cbasViewer

import ~cbassuser/cbass/gcpCbass/control/sch/schedlibSouth.sch


# start observation
init_obs("Beginning schedule: radio_pointing_Sa.sch")

# do a sky dip
#do_sky_dip

# Set archiving to the correct mode
mark remove, all

#---------------------------------------------------------------------------------
# Main loop -- track the specified source, do a pointing cross, repeat until end of time
#---------------------------------------------------------------------------------

log "Waiting for utcstart..."
until $after($utcstart,utc) 

while ($time(utc)<$utcstop){

	#Extra check on elevation
	if ($elevation($source) <= 27) {
        log "source too low for pointing cross"  
        break
        }

	log "Doing scan on ",$source

      	# Do scan
      	do_point_scan($source)
	zeroScanOffsets   	# zero offsets
    

    	# Extra break if past utcstop
    	if $time(utc)>$utcstop {
       	log "utcstop reached"
       	break
        }   

         
}
    
  

# do a sky dip
#do_sky_dip

terminate("Finished schedule: radio_pointing_Sa.sch")

