(Interval time)

	# set the cryocontrol to PID
	setCryoControlType 1,1 
	#set the loop range to low
	setCryoLoopRange 1,0
while($elapsed<$time){

	# turn the noise source on
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	
	until $elapsed>3m
}
