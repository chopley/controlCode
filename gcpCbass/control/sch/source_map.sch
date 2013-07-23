(Source cal_src, Source targ_src, Count nreps, Interval cal_time, Interval targ_time)

# Flip between a cal source and nearby target source.
# You specify how long to look at each one and how many cycles
#
# Initially cal source needs to be a bright point source
#
# e.g.:
# source_map.sch(J0530+135,taua,20,5m,10m)  VLA says 2.3Jy @ 7mm
# source_map.sch(J2015+371,cyga,20,5m,10m)  3.7Jy @ 86GHz
# source_map.sch(J0102+584,casaoff,20,5m,10m)  VLA says 2.4Jy @ 7mm
# source_map.sch(3c279,3c273,20,5m,10m)
# source_map.sch(3C286,A1914,20,5m,10m)
# source_map.sch(J1337-129,RXJ1347-11,24,5m, 10m) 2.6Jy

import /home/szadaq/sza/array/sch/schedlib.sch

resetSystemState "Starting schedule source_map.sch"

# setup the downconverter AGCs
setDownconAGC 

# do az rotation to monitor tilt
spinAz

# check radio pointing by doing a cross on a bright source
# (in principle should pick appropriate source from a list based on LST)
doPointCross $targ_src

# do a track on a bright stable (or calculable) source to monitor gain
# (in principle should pick appropriate source from a list based on LST)
#doAmpCalObs

do Count i=1,$nreps,1 {

  # Observe cal source (note: observe does Psys cal as well)
  observe $cal_src, f1, $cal_time

  # Observe target
  observe $targ_src, f0, $targ_time
}

# Provide bracketing cal obs
observe $cal_src, f1, $cal_time

resetSystemState "Ending schedule source_map.sch"