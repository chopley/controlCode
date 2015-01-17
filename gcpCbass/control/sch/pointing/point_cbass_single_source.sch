(Source source, Time lststart, Time lststop, Count npoints, PointingOffset azOffset, PointingOffset elOffset, Double snr) 
# sourc name, start time, stop time, number of points to record on each star, original az offset, original el offset, snr of star


# Get rid of any legacy signals.
signal/init done
mark remove, all

# Archive only marked frames
archive combine=1, filter=false
open grabber

model optical, ptel=0
offset az=$azOffset, el=$elOffset
sky_offset x=0, y=0
radec_offset ra=0, dec=0

 
# Configure the frame grabber 
setDefaultFrameGrabberChannel chan1
configureFrameGrabber chan=chan1
setOpticalCameraRotation angle=278
configureFrameGrabber flatfield=none
setOpticalCameraFov fov=120, chan=chan1
setOpticalCameraAspect aspect=0.75, chan=chan1
setOpticalCameraXImDir upright, chan=chan1
setOpticalCameraYImDir upright, chan=chan1
addSearchBox 8, 9, 501, 472, true, chan=chan1

open grabber


    while ($time(lst)<$lststop) {


		#go to source
		track $source
		
  		until $acquired(source) | $elapsed>30s

		# Get an image
  		grabFrame chan=chan1

		# Check if we have a star within 10arcmin


  		if($imstat(snr) > $snr & $peak(xabs) < 00:30:00 & $peak(yabs) < 00:30:00) {
     			print "Found star at ",$peak(x),",",$peak(y)," with snr ",$imstat(snr)
     			center # offset to star
     			until $acquired(source) | $elapsed>30s
     			until $elapsed>5s # always wait for the telescope to settle after center

     			# Now record a few points to sample the seeing disk
     			do Count i=1,$npoints,1 {
				grabFrame chan=chan1
				until $elapsed>3s

			if($imstat(snr) > $snr & $peak(xabs) < 00:30:00 & $peak(yabs) < 00:30:00) {
	  			print "Star offset ",$peak(x),",",$peak(y),", snr ",$imstat(snr)
	  			center 
	  			until $acquired(source) | $elapsed > 30s
	  			until $elapsed>3s
          			mark add, f0
	  			until $elapsed>3s
	  			mark remove, f0
			} else {
	  		print "No star"
			}
     			}  # end of do loop
  		} else {   # star not withint 10 arcmin
    			print "Failed to find star"
  		}  # done with star finding commands
} # end check of whether to go to certain star

cleanup {
  offset az=0, el=0
  archive combine=1, filter=false
  model radio
}
