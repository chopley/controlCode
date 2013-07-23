(Interval time)

mark remove, f12
generalRoachCommand NDIODEXXXX,0,0
setCryoSkyTemp 1,12.0
until $elapsed>10m
mark add, f12
generalRoachCommand NDIODEXXXX,1,0
until $elapsed>1m
mark remove, f12
generalRoachCommand NDIODEXXXX,0,0
until $elapsed>$time
mark add, f12
generalRoachCommand NDIODEXXXX,1,0
until $elapsed>1m
mark remove, f12
generalRoachCommand NDIODEXXXX,0,0
	
