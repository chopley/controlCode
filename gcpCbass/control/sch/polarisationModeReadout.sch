
generalRoachCommand CHANGEMODE,1,0
until $elapsed>1s
generalRoachCommand CHANGEMODE,1,1
until $elapsed>10s

generalRoachCommand NDIODEXXXX,1,0
until $elapsed>10s
generalRoachCommand NDIODEXXXX,0,0
	
