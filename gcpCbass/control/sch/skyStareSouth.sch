(Interval time,Interval noiseTime)

import ~cbassuser/cbassSaControl/cbass/gcpCbass/control/sch/schedlibSouth.sch
init_obs("beginning schedules skyStareSouth")
setCryoSkyTemp 1,12.0
zeroScanOffsets
encoder_limits -270,130,10,86,0,0
slew az=0, el=85
until $acquired(source)|$elapsed>90s
slew az=-90, el=85
until $acquired(source)|$elapsed>90s

while($elapsed<$time){

	# turn the noise source on
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	generalRoachCommand NDIODEXXXX,0,0
	
	until $elapsed>$noiseTime
}
slew az=-90, el=83
encoder_limits -270,130,10,84,0,0
setCryoSkyTemp 1,12.0
terminate("Ending schedule: skyStareSouth")
