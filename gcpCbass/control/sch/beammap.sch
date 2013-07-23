(Source src, PointingOffset maxOff, Sexagesimal del)

# schedule to do a big beam map.
# the offsets are in elevation -- does a scan in azimuth
import ~cbass/gcpCbass/control/sch/schedlib.sch

# initialize
init_obs("Beginning beam map schedule")


# do sky dip
slew az=0
until $acquired(source)
do_sky_dip
until $elapsed>3s

# blank stare for mains
mark add, f4
until $elapsed>2m
mark remove, f4
offset az=0



noise_on(10s)


# track the source, then do a big raster 
track $src
until $acquired(source)

# Raster scan in x and y

PointingOffset dStart = -$maxOff
PointingOffset dStop  = $maxOff+0.01

if($elevation($src) > 30) { 
	do PointingOffset dy = $dStart, $dStop, $del {


	   	offset el=$dy  # apply the offset
	   	until $elapsed>1s
	   	until $acquired(source)  # wait for offset
           	until $elapsed>2s        # wait for settle
		
		# turn the noise source on
		noise_on(5s)

		# do a scan in azimuth
		mark add, f9
		scan az-1ds-20d-full-single-scan
		until $elapsed>80s
		mark remove, f9

		# zero the offsets
		zeroScanOffsets
		until $acquired(source)

		# turn the noise source on
		noise_on(5s)
	}
	
	offset az=0, el=0  # Reset it.
	until $acquired(source)
} else if ($elevation($src) < 15) {
	log "source too low"
}

# do a sky dip to end
do_sky_dip

# finish observation. 
terminate("done with beam map schedule")	
