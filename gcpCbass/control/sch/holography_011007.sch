# Reset the dfmux because it hangs after about 9 hours
import /home/sptdaq/gcpDeploy/gcp/control/sch/reset_dfmux.sch
until $elapsed > 20s

(Source target)

#-------------------------------------------------------------
# HOLOGRAPHY_011007.SCH
# 1 Oct 2007 - JV
#
# What we want to do is do a scan at 5 arcmin/sec = 0.083 d/s for 128 arcmin = 2.13 d
# use scan file holo-2p13d-0p083ds-0p52dss.scan with AZ offset = 1.0792
# takes 26 seconds, 98% duty cycle
# slew back to field center and stare for 5 seconds
# EL step = 1 arcmin
#
# we need to set up the coordinates of the IceCube counting facility = ice3cf
#
# f8 = optical pointing
#
#--------------------------------------------------------------


# import the file we always need
import ~sptdaq/gcproot/control/sch/schedlib.sch

# always do at beginning of observation
init_obs("starting holography_011007.sch")

# slew to target
track $target
until $acquired(source)

# what flag to use? optical pointing?
mark add, f8
until $acquired(mark)

log "Starting holography map ... : ", $date

do PointingOffset del = -1:4, 1:4, 0:1
{

	# set EL offset
	log "Moving to EL offset = ", $del
	
	# do continuous half-scan 
	offset az=-1.0792
	until $acquired(source)
	offset el=$del
	until $acquired(source)
        log "Settle before starting scan", $date
	until $elapsed > 40s
	scan  holo-2p13d-0p083ds-0p52dss
	until $acquired(scan)

	# go back to field center and stare for 5 seconds
  	#zeroScanOffsets
	offset az=0
	until $acquired(source)
	offset el=0
	until $acquired(source)
	log "Staring at center :", $date
	until $elapsed > 40s
	
}

log "Finished holography map ... : ", $date

mark remove, f8
until $acquired(mark)

# always do at end of observation
terminate("finished holography_011007.sch")

halt

