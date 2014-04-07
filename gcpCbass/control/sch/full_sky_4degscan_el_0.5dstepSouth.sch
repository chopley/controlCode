import ~cbassuser/cbass/gcpCbass/control/sch/schedlibSouth.sch
scan_catalog ~/cbass/gcpCbass/control/scan/cbass_scan.cat
#setCryoSkyTemp 1, 6.5

init_obs("Starting schedule to map the sky in az and el")
halt
noise_on(5s)
# these first steps are to ensure that the telescope is at the right
# place in the wrap
encoder_limits -270,130,5,85,0,0
slew az=0, el=20
until $acquired(source)|$elapsed>60s
slew az=-100, el=20
until $acquired(source)|$elapsed>60s
slew az=-250, el=20
until $acquired(source)|$elapsed>60s

do PointingOffset elVal = 5.5, 83.1, 0.5 {
    engageServo on
   slew az=-250, el=$elVal
  # slew el=$elVal
   until $acquired(source)|$elapsed>60s
   
  #noise_on(5s)

   mark add, f0
   # now we scan a full az scan at 4 deg/s
   scan az-4ds-360d-full-single-scan
   until $acquired(scan)|$elapsed>240s
   mark remove, f0

   noise_on(5s)

   zeroScanOffsets
}
encoder_limits -270,130,15,85,0,0
init_obs("Ending schedule to map the sky in az and el")
