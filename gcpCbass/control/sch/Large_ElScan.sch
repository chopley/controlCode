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
# Track source, do a raster
#--------------------------

# Track Source
track $source_name
until $acquired(source)|$elapsed>30s

noise_on_general 5s, 8s

# Start by going to opposite az point in sky and do elevation scan

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

# take it back round correctly in the wrap ready to track the source
# and do another elevation scan at the correct az position
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
