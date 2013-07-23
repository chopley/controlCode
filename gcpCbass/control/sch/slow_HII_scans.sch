# Schedule to do slow rasters on Mat5a and RCW38 for calibration purposes.
#
# scan files used:
# az-2p2d-0p325ds-1dss.scan for rcw38
# az-3p1d-0p458ds-1dss.scan for mat5a
# 
# azimuth offsets:
# -1.1992500 for rcw38 
# -1.7495600 for mat5a
#
# elevation steps are 0.002222,  between el=-0.696667,el=0.696667.  Each scan
# over a source takes 627 of these (~17 seconds each scan) so the whole thing 
# will take about 6 hours to complete.
#
# Author:  KKS
#
# there is no fancy business in this script, it just does both sources
# without defining any new commands


# import the file we always need
import ~sptdaq/gcproot/control/sch/schedlib.sch


# do the initialization and setup we always want
init_obs("starting slow_HII_scans.sch")
calibrator_warm_up
calibrator_6Hz
set_up_radio

###########################3
# we will do rcw38 first
# call the "acquire_source" function, which also does a fine_null and 
# cal/elnod pair
acquire_source(rcw38)

#offset by half of scan width:
offset az=-1.1992500
until $acquired(source)

#turn on f0
mark add, f0
until $acquired(mark)

#do the scan:
do PointingOffset eloff=-0.696667,0.696667,0.002222 {
	offset el=$eloff
	until $acquired(source)
	scan az-2p2d-0p325ds-1dss
	until $acquired(scan)
}

#remove f0 bit
mark remove, f0
until $acquired(mark)
halt

######################
#now do the same thing for mat5a

acquire_source(mat5a)

#offset by half of scan width:
offset az=-1.7495600
until $acquired(source)

#turn on f0
mark add, f0
until $acquired(mark)

#do the scan:
do PointingOffset eloff=-0.696667,0.696667,0.002222{
	offset el=$eloff
	until $acquired(source)
	scan az-3p1d-0p458ds-1dss
	until $acquired(scan)
}

#remove f0 bit
mark remove, f0
until $acquired(mark)
halt


#cleanup:
track current
terminate("finished slow_HII_scans.sch")
#calibrator_OFF
offset az=0, el=0
mark remove, all
until $acquired(mark)
halt


