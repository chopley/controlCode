(Interval time)

while($elapsed<$time){
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s

	# turn the noise source on
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	
	setCryoSkyTemp 1,12.0
	until $elapsed>20m
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	until $elapsed>1m
	
	setCryoSkyTemp 1,15.0
	until $elapsed>20m
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	until $elapsed>1m

	setCryoSkyTemp 1,18.0
	until $elapsed>20m
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	until $elapsed>1m
	
	setCryoSkyTemp 1,25.0
	until $elapsed>20m
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	until $elapsed>1m

	
	
#	setCryoSkyTemp 1,50.0
#	until $elapsed>20m
#	generalRoachCommand NDIODEXXXX,1,0
#	until $elapsed>10s
#	generalRoachCommand NDIODEXXXX,0,0
#	until $elapsed>10s
#	until $elapsed>1m
	
#	setCryoSkyTemp 1,77.0
#	until $elapsed>20m
#	generalRoachCommand NDIODEXXXX,1,0
#	until $elapsed>10s
#	generalRoachCommand NDIODEXXXX,0,0
#	until $elapsed>10s
#	until $elapsed>1m
	
	setCryoSkyTemp 1,12.0
	until $elapsed>20m
}
