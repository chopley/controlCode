import ~cbass/gcpCbass/control/sch/schedlib.sch

init_obs("Starting schedule to step through temperatures")

slew el=83
until $acquired(source)

do Double tempVal=4.5,10.01,0.5 {

	setCryoSkyTemp 1, $tempVal
	mark add, f4
	until $elapsed > 5m

	noise_on(1m)

	mark remove, f4
}

# set the temperature back
setCryoSkyTemp 1, 6.5


terminate("Ending tempStepping.sch")
	
 	

