(Source source_name)
# import the schedlib 
import ~cbassuser/cbass/gcpCbass/control/sch/schedlibSouth.sch

#get the scan catalogue
scan_catalog ~cbassuser/cbass/gcpCbass/control/scan/cbass_scan.cat

# initialize
init_obs("beginning schedules colltower beam map")
slew az=0,el=30
encoder_limits -270,130,5,85,0,0
until $acquired(source)|$elapsed>30s

# Set archiving to the correct mode
mark remove, all

#--------------------------
# Track source, do a 360 degree Az scan
#--------------------------

# Track Source
track $source_name
until $acquired(source)|$elapsed>30s

noise_on_general 5s, 8s

# Now do a 360 degree az scan 

#
# these first steps are to ensure that the telescope is at the right
# place in the wrap

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
   scan az-4ds-360d-full-single-scan, nreps=2
  # until $acquired(scan)|$elapsed>240s

noise_on_general 5s, 8s

mark remove,f7
zeroScanOffsets

# take it back round correctly in the wrap
slew az=-100
until $acquired(source)|$elapsed>40s
slew az=0
until $acquired(source)|$elapsed>20s

track $source_name
until $acquired(source)|$elapsed>40s

noise_on_general 5s, 8s

# Should now be back on source

encoder_limits -270,130,15,85,0,0
terminate("Ending schedule: LARGE, 360deg az scan")
