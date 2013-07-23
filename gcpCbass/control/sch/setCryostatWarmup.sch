
	# set the cryocontrol to manual
	setCryoControlType 1,2 
	#set the loop range to high
	setCryoLoopRange 1,2
	#Set the output to 20% of maximum
	setCryoPowerOutput 1,20

	# turn the noise source on
	mark add, f12
	generalRoachCommand NDIODEXXXX,1,0
	until $elapsed>10s
	mark remove, f12
	generalRoachCommand NDIODEXXXX,0,0
	
	until $elapsed>3m
