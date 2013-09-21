(Interval time,Double startTemp,Double dTemp,Double endTemp)
import ~cbassuser/cbassSaControl/cbass/gcpCbass/control/sch/schedlibSouth.sch
## time is for the time at each temp
##startTemp is the beginning temp
##end Temp is the end Temp
##dTemp is the temperature step
##total time is ~[1+(endTemp-startTemp)/dTemp]*time
init_obs("beginning schedules coldLoadStep")
zeroScanOffsets
encoder_limits -270,130,10,86,0,0
slew az=0, el=85
until $acquired(source)|$elapsed>90s
slew az=-90, el=85
until $acquired(source)|$elapsed>90s


Double Temp=$startTemp
while($Temp<=$endTemp){
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s

	# turn the noise source on
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	
	setCryoSkyTemp 1,$Temp
	until $elapsed>$time
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	until $elapsed>1m
	Temp=$Temp+$dTemp
}
setCryoSkyTemp 1,12
slew az=-90, el=83
encoder_limits -270,130,10,84,0,0
terminate("Ending schedule: coldLoadStep")

