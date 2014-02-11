(Interval time)

import ~cbassuser/cbass/gcpCbass/control/sch/schedlibSouth.sch
init_obs("beginning schedules phaseShifterCalibration")
zeroScanOffsets
encoder_limits -270,130,10,86,0,0
slew az=0, el=85
until $acquired(source)|$elapsed>90s
slew az=-90, el=85
until $acquired(source)|$elapsed>90s
while($elapsed<$time){

setCryoSkyTemp 1,12
until $elapsed>10m
generalRoachCommand PHASESHIFT, 0 ,1
until $elapsed>10s
do Double i = 0, 63, 1{ 
	# turn the noise source on
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	generalRoachCommand PHASESHIFT, $i ,1
	until $elapsed>10s
	}

}
generalRoachCommand PHASESHIFT, 0 ,1
