import ~cbassuser/gcpCbass/control/sch/schedlibSouth.sch

setCryoSkyTemp 1, 6.5

init_obs("Starting schedule to step through elevations")


encoder_limits -270,130,5,85,0,0
slew az=0, el=20
until $acquired(source)|$elapsed>60s
slew az=-100, el=20
until $acquired(source)|$elapsed>60s
slew az=-250, el=20
until $acquired(source)|$elapsed>60s

do PointingOffset elVal = 16, 83.1, 1 {

   slew az=0, el=$elVal
   until $acquired(source)
   mark add, f0


   # now we scan a full az scan at 2 deg/s
   scan az-2ds-360d-full-single-scan
   until $acquired(scan)|$elapsed>7m
   mark remove, f0

   zeroScanOffsets
}
