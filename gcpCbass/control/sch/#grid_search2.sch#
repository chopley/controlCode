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

if($elevation($src) > 15) { 
	do PointingOffset dy = $dStart, $dStop, $del {
		do PointingOffset dx = $dStart, $dStop, $del {

		log $dx, $dy

		offset az=$dx, el=$dy  # apply the offset
		until $acquired(source)  # wait for offset
		until $elapsed>2s        # wait for settle
		
		newFrame
		until $acquired(frame)  

		mark add, f6
		until $elapsed >3s
		mark remove, f6

		newFrame
		until $acquired(frame)  
		}
	}
	
	offset az=0, el=0  # Reset it.
	until $acquired(source)
} else if ($elevation($src) < 15) {
	log "source too low"
}

 
	
