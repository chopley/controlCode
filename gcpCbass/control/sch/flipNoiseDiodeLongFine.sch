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
	until $elapsed>60m
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	until $elapsed>1m
	
	setCryoSkyTemp 1,14.0
	until $elapsed>60m
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	until $elapsed>1m
	
	setCryoSkyTemp 1,16.0
	until $elapsed>60m
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	until $elapsed>1m

	setCryoSkyTemp 1,18.0
	until $elapsed>60m
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	until $elapsed>1m
	
	setCryoSkyTemp 1,20.0
	until $elapsed>60m
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	until $elapsed>1m
	
	setCryoSkyTemp 1,22.0
	until $elapsed>60m
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	until $elapsed>1m

	
	setCryoSkyTemp 1,24.0
	until $elapsed>60m
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	until $elapsed>1m	

	setCryoSkyTemp 1,26.0
	until $elapsed>60m
	generalRoachCommand NDIODEXXXX,1,0
	mark add, f12
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	until $elapsed>1m
	
	setCryoSkyTemp 1,28.0
	until $elapsed>60m
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	until $elapsed>1m

	setCryoSkyTemp 1,30.0
	until $elapsed>60m
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	until $elapsed>10s
	until $elapsed>1m

	
	setCryoSkyTemp 1,12.0
	until $elapsed>20m
}
