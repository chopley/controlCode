(Interval time)

setCryoSkyTemp 1,12
until $elapsed>10m

while($elapsed<$time){
	
	# turn the noise source on
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	generalRoachCommand NDIODEXXXX,0,0
	
	until $elapsed>30m
}
