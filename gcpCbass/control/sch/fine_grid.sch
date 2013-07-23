(Source src, PointingOffset maxOff, Sexagesimal del)

# schedule to do a big grid search.

# set everything up
enableRxSimulator false
enableAltWalshing false
enableWalshing true


# track the source, then do a big raster pattern.
track $src
until $acquired(source)

# Raster scan in x and y

PointingOffset dStart = -$maxOff - $del
PointingOffset dStop  = $maxOff

if($elevation($src) > 20) { 
	do PointingOffset dy = $dStart, $dStop, $del {
		do PointingOffset dx = $dStart, $dStop, $del {
		
		log $dx

		sky_offset x=$dx, y=0  # apply the offset
		until $acquired(source)  # wait for offset

		newFrame
		until $acquired(frame)  

		mark add, f6
		until $elapsed >10s
		mark remove, f6

		newFrame
		until $acquired(frame)  
		}
	}
	
	sky_offset x=0, y=0  # Reset it.
	until $acquired(source)
} else if ($elevation($src) < 20) {
	log "source too low"
}

 
	
