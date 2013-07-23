(Source src, PointingOffset maxOff, Sexagesimal del)

# schedule to do a big grid search.

# set everything up
enableRxSimulator false
enableAltWalshing true
enableWalshing true


# track the source, then do a big raster pattern.
track $src
until $acquired(source)

# Raster scan in x and y

PointingOffset dStart = -$maxOff - $del
PointingOffset dStop  = $maxOff
open grabber


if($elevation($src) > 15) { 
	do PointingOffset dx = 5, 2, -0.5 {
		do PointingOffset dy = -5, 5, 0.3 {

		log $dx, $dy

		offset az=$dx, el=$dy  # apply the offset
		until $acquired(source)  # wait for offset
		until $elapsed>2s        # wait for settle
		
		newFrame
		until $acquired(frame)  

	        grabFrame chan=chan1
		until $elapsed>3s


		newFrame
		until $acquired(frame)  
		}
	}
	
	offset az=0, el=0  # Reset it.
	until $acquired(source)
} else if ($elevation($src) < 30) {
	log "source too low"
}

 
	
