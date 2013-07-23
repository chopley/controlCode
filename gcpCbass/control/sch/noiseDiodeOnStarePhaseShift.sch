(Interval time)

generalRoachCommand NDIODEXXXX,0,0
setCryoSkyTemp 1,12.0
until $elapsed>10m
generalRoachCommand NDIODEXXXX,1,0
until $elapsed>1m
generalRoachCommand NDIODEXXXX,0,0
until $elapsed>1m
generalRoachCommand NDIODEXXXX,1,0
while($elapsed<$time){
	do Double i = 0, 63, 1{ 
		generalRoachCommand PHASESHIFT, $i ,1
		until $elapsed>20s
		}
}
generalRoachCommand NDIODEXXXX,0,0
until $elapsed>1m
generalRoachCommand NDIODEXXXX,1,0
until $elapsed>1m
generalRoachCommand NDIODEXXXX,0,0
	
