(Interval time,Interval noiseTime)

setCryoSkyTemp 1,12.0

while($elapsed<$time){

	# turn the noise source on
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	generalRoachCommand NDIODEXXXX,0,0
	
	until $elapsed>$noiseTime
}
