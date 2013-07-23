(Interval time,Double startTemp,Double dTemp,Double endTemp)
## time is for the time at each temp
##startTemp is the beginning temp
##end Temp is the end Temp
##dTemp is the temperature step
##total time is ~[1+(endTemp-startTemp)/dTemp]*time
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

