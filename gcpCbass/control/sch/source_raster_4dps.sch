(Interval time, Source source_name)

# import the schedlib 
import ~cbassuser/cbass/gcpCbass/control/sch/schedlibSouth.sch

#get the scan catalogue
scan_catalog ~cbassuser/cbass/gcpCbass/control/scan/cbass_scan.cat

# initialize
init_obs("beginning schedules source_raster_4dps")
# NB - commented out by MP on 21 Jan, not sure why these are needed?
#slew az=0,el=30
#until $acquired(source)|$elapsed>3m
#slew az=-100
#until $acquired(source)|$elapsed>1m
# track SCP
track scpfixed
until $acquired(source)|$elapsed>1m

# Noise on 
noise_on_general 5s, 8s

# do a sky dip
do_large_sky_dip


# slew to start location
track $source_name
until $acquired(source)|$elapsed>3m

# Set archiving to the correct mode
mark remove, all

#-----------------------------------------------------------------------
# Main loop -- track the next source,do a raster, move on
#-----------------------------------------------------------------------

while($elapsed<$time) {

  noise_on_general 5s, 8s

#  if($elevation($source_name) >= 15) {

      # Track Source
      track $source_name
      until $acquired(source)|$elapsed>1m

      # Do scan
      mark add, f9	
      scan source_raster_4dps, nreps=1 
      until ( $acquired(scan) | ($elapsed > 10m) )
      mark remove, f9

      zeroScanOffsets   	# zero offsets
      until $elapsed>10s

#  }
}

terminate("Ending schedule: source_raster_4dps")
