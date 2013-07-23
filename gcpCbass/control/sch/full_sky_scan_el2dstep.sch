import ~cbass/gcpCbass/control/sch/schedlib.sch

setCryoSkyTemp 1, 6.5

init_obs("Starting schedule to step through elevations")


# these first steps are to ensure that the telescope is at the right
# place in the wrap
slew az=165
until $acquired(source)
slew az=0
until $acquired(source)


do PointingOffset elVal = 16, 83.1, 1 {

   slew az=0, el=$elVal
   until $acquired(source)
   mark add, f0


   # now we scan a full az scan at 2 deg/s
   scan az-2ds-360d-full-single-scan
   until $acquired(scan)
   mark remove, f0

   zeroScanOffsets
}
