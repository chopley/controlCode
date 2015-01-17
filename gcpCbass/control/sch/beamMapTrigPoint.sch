(Source source_name)
# import the schedlib 
import ~cbassuser/cbass/gcpCbass/control/sch/schedlibSouth.sch

#get the scan catalogue
scan_catalog ~cbassuser/cbass/gcpCbass/control/scan/cbass_scan.cat

# initialize
init_obs("beginning schedules colltower beam map")
slew az=0,el=30
encoder_limits -270,130,2,85,0,0
until $acquired(source)|$elapsed>30s

# Set archiving to the correct mode
mark remove, all

#--------------------------
# Track source, do a raster
#--------------------------

# Track Source
track $source_name
until $acquired(source)|$elapsed>30s

noise_on_general 5s, 8s

# Do scan
mark add, f9	
# scan source_raster_8x4x0.05, nreps=1 
# until ( $acquired(scan) | ($elapsed > 21m) )

# If the previous scan is too wide, try this one instead.  This scan
# file also goes back and forth in az for each el step.
scan source_raster_8x4x0.05South, nreps=1
until ( $acquired(scan) | ($elapsed > 14m) )

mark remove, f9

zeroScanOffsets   	# zero offsets
until $elapsed>10s
#
# Now do a 360 degree az scan 

#
# these first steps are to ensure that the telescope is at the right
# place in the wrap
encoder_limits -270,130,2,85,0,0
noise_on_general 5s, 8s

mark add, f7
slew az=0
until $acquired(source)|$elapsed>60s
slew az=-100
until $acquired(source)|$elapsed>60s
slew az=-250
until $acquired(source)|$elapsed>60s

noise_on_general 5s, 8s

# now we scan a full az scan at 4 deg/s
   scan az-4ds-360d-full-single-scan
   until $acquired(scan)|$elapsed>240s

noise_on_general 5s, 8s

mark remove,f7
zeroScanOffsets

# now go back to the trig point opposite point and slew back over in elevation

offset az=-180
track $source_name
until $acquired(source)|$elapsed>60s

noise_on_general 5s, 8s

mark add, f1

slew el=85
until $acquired(source)|$elapsed>100s

noise_on_general 5s, 8s

slew el=5
until $acquired(source)|$elapsed>100s
mark remove, f1
offset az=0

# take it back round correctly in the wrap
slew az=-100
until $acquired(source)|$elapsed>40s
slew az=0
until $acquired(source)|$elapsed>20s

track $source_name
until $acquired(source)|$elapsed>40s

noise_on_general 5s, 8s

mark add, f1
slew el=85
until $acquired(source)|$elapsed>100s

noise_on_general 5s, 8s

slew el=5
until $acquired(source)|$elapsed>100s

noise_on_general 5s, 8s

mark remove, f1

# Should now be back on source
track $source_name

encoder_limits -270,130,15,85,0,0
terminate("Ending schedule: colltower beam map")
