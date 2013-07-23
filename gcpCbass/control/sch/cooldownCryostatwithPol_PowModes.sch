(Interval time)

	# set the cryocontrol to PID
	setCryoControlType 1,1 
	#set the loop range to low
	setCryoLoopRange 1,0
	
	generalRoachCommand CHANGEMODE,1,0
	until $elapsed>1s
	generalRoachCommand CHANGEMODE,1,1
	until $elapsed>10s

	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	generalRoachCommand NDIODEXXXX,0,0
		# turn the noise source on
		mark add, f12
		generalRoachCommand NDIODEXXXX,1,0
		until $elapsed>10s
		mark remove, f12
		generalRoachCommand NDIODEXXXX,0,0
		
		until $elapsed>3m

	generalRoachCommand CHANGEMODE,2,0
	generalRoachCommand CHANGEMODE,2,1
	until $elapsed>10s

	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	generalRoachCommand NDIODEXXXX,0,0
		# turn the noise source on
		mark add, f12
		generalRoachCommand NDIODEXXXX,1,0
		until $elapsed>10s
		mark remove, f12
		generalRoachCommand NDIODEXXXX,0,0
		
		until $elapsed>3m

while($elapsed<$time){

generalRoachCommand CHANGEMODE,1,0
until $elapsed>1s
generalRoachCommand CHANGEMODE,1,1
until $elapsed>10s

generalRoachCommand NDIODEXXXX,1,0
until $elapsed>10s
generalRoachCommand NDIODEXXXX,0,0
do Double i=0,5,1{	
	# turn the noise source on
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	
	until $elapsed>3m
	}
generalRoachCommand CHANGEMODE,2,0
generalRoachCommand CHANGEMODE,2,1
until $elapsed>10s

generalRoachCommand NDIODEXXXX,1,0
until $elapsed>10s
generalRoachCommand NDIODEXXXX,0,0
do Double i=0,5,1{	
	# turn the noise source on
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	
	until $elapsed>3m
	}
}
