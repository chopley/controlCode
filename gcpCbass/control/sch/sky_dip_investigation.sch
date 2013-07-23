import ~cbass/gcpCbass/control/sch/schedlib.sch
scan_catalog ~cbass/gcpCbass/control/scan/cbass_scan.cat


# first we reset everything
init_obs("Starting sky dip schedule")

# first we go to an az of zero
slew az=179, el=74
until $acquired(source)

slew az=0, el=74
until $acquired(source)

# now we go around the circle and do a sky dip at each az value

# start stepping through azimuth (15 degrees)
do PointingOffset azval = 0, 360, 15
{
   slew az=$azval, el=74
   until $acquired(source)   	# wait for it to get there
   noise_on(5s)			# put noise source on before scan
	
   # do a sky dip
   do_sky_dip

   # another noise source
   noise_on(5s)
   mark remove, f9

   # done with this elevation
   zeroScanOffsets
   until $acquired(source)

}

# clear everything
terminate("Ending sky dip investigation Schedule")
