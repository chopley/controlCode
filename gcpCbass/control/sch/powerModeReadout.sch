generalRoachCommand CHANGEMODE,2,0
generalRoachCommand CHANGEMODE,2,1
until $elapsed>10s

generalRoachCommand NDIODEXXXX,1,0
until $elapsed>10s
generalRoachCommand NDIODEXXXX,0,0
	
