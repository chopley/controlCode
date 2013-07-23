# radio pointing to measure positions of rcw38 and mat5a relative to some quasars

listof Source sources = {
  0537-441,
  0208-512,
  0208-512,
  0521-365,
  0537-441,
  0521-365,
  rcw38,
  mat5a,
  0537-441,
  0521-365,
  0537-441,
  0521-365,
  0537-441,
  0521-365
}

# import the file we always need
import ~sptdaq/gcproot/control/sch/schedlib.sch

#########################################################
command do_cal(Interval cal_time) {
calibrator_open_shutter
mark add, f0
until $acquired(mark)
until $elapsed>$cal_time
mark remove, f0
until $acquired(mark)
calibrator_close_shutter
}
#########################################################
command fast_point_el61(Source target) {

track $target
until $acquired(source)
fine_null
do_el_nod
do_cal 1m

offset az=-1.496
until $acquired(source)

mark add, f0
until $acquired(mark)

do PointingOffset eloff= -0.53,0.53,0.00833 {
  offset el=$eloff
  until $acquired(source)
  scan az-2p4d-0p55ds-1dss
  until $acquired(scan)
  if($regVal("array.fridge.Temperature[2]")>0.7) {
	print "fridge has blown"
	abort_schedule
  }
}

mark remove, f0
until $acquired(mark)

}
#########################################################
command clean_up_schedule() {
terminate("finished spt_point_radio.sch")
#calibrator_OFF
track north55
}
#########################################################

# always do at beginning of observation
init_obs("starting spt_point_radio.sch")

calibrator_warm_up
calibrator_6Hz
set_up_radio

foreach(Source s) $sources {
  fast_point_el61 $s 
}

cleanup {
  clean_up_schedule
}

