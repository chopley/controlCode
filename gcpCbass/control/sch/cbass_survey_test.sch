(Time lststop)

# test schedule of cbass survey schedule
# does a full scan in azimuth, does noise source
import ~cbass/gcpCbass/control/sch/schedlib.sch

# initialize
init_obs("beginning of cbass survey test schedule")

# do a sky dip
do_sky_dip

# slew to the start location.
slew az=179, el=37
until $acquired(source)
slew az=0, el=37
until $acquired(source)
until $elapsed>10s

while($time(lst) < $lststop) { 
	
	# turn the noise source on
	noise_on(5s)

	# do a scan in azimuth
	mark add, f0
	scan az-4ds-360d-full-single-scan
	until $elapsed>246s
	mark remove, f0

	# turn the noise source on
	noise_on(5s)
}

# do a sky dip to end
do_sky_dip

# finish observation. 
terminate("done with test of cbass survey schedule")
