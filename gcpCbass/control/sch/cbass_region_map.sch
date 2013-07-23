(Source source, Count nreps)

# a schedule to map regions around a given source (much like the cygaregion map)
# I say we use this for commmissioning of specific regions.
# Regions of interest (with many sources) are:
#
# cygaOff  ; RA 20:30
# J0534+219; RA 05:30 -- this is Taua
# J0627-058; RA 06:30
# 87GBJ1858+0108; RA 19:00
# MG20535+220; RA 20:53
#
#  nreps is the number of times you want to do the grid
#  should take about 90 minutes for nreps = 1

# import the schedlib
import ~cbass/gcpCbass/control/sch/schedlib.sch

#get the scan catalogue
scan_catalog ~cbass/gcpCbass/control/scan/cbass_scan.cat

# initialize
init_obs("beginning schedules large region map")

# blank stare for mains
mark add, f4
until $elapsed>2m
mark remove, f4
offset az=0



noise_on(10s)

# slew to start location
track $source
until $acquired(source)

# Set archiving to the correct mode
mark remove, all


do Count i=1,$nreps,1 {

  # do a sky dip
  offset/add az=5  # slew off source for a sky dip
  until $acquired(source)
  do_sky_dip
  offset/add az=-5
  until $acquired(source)

	

  # slew to start location
  zeroScanOffsets
  offset az=0, el=0
  sky_offset x=0, y=0
  track $source
  until $acquired(source)

  # fire the noise source
  noise_on(10s)



  # Do scan  -- This scan takes about 
  # scan steps in elevation by 0.5 deg (from low to high elev), 
  #    does a 30 deg scan in azimuth at 1 deg/s
  #    takes 75 minutes to complete.

  if $elevation($source) > 30 {
    mark add, f0
    log "starting scan"
    scan square-field-15deg-az
    until $acquired(scan)
    mark remove, f0
  } else {
    log "Source too low"
  }
	
}

# finish with a sky dip and a noise source
offset/add az=5  # slew off source for a sky dip
until $acquired(source)
do_sky_dip
offset/add az=-5
until $acquired(source)

# fire the noise source
noise_on(5s)

# finish observation. 
terminate("done map of region")
