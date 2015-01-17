# import the schedlib 
import ~cbassuser/cbassSaControl/cbass/gcpCbass/control/sch/schedlibSouth.sch

#get the scan catalogue
scan_catalog ~cbassuser/cbassSaControl/cbass/gcpCbass/control/scan/cbass_scan.cat

# initialize
init_obs("beginning schedules colltower beam map")
slew az=0,el=30
encoder_limits -270,130,5,85,0,0
until $acquired(source)|$elapsed>30s

# Set archiving to the correct mode
mark remove, all

#--------------------------
# Track source, do a raster
#--------------------------

# Track Source
track colltower
until $acquired(source)|$elapsed>30s

noise_on_general 5s, 8s

# Do scan
mark add, f9	
# scan source_raster_8x4x0.05, nreps=1 
# until ( $acquired(scan) | ($elapsed > 21m) )

# If the previous scan is too wide, try this one instead.  This scan
# file also goes back and forth in az for each el step.
scan source_raster_5x4x0.05_1ds, nreps=1
until ( $acquired(scan) | ($elapsed > 14m) )

mark remove, f9

zeroScanOffsets   	# zero offsets
until $elapsed>10s

encoder_limits -270,130,15,85,0,0
terminate("Ending schedule: colltower beam map")
