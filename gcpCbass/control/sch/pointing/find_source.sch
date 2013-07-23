(Source src)

track $src

# Get rid of any legacy signals.
signal/init done
mark remove, all

# Archive only marked frames
archive combine=1, filter=false
open grabber

model optical, ptel=0
sky_offset x=0, y=0
radec_offset ra=0, dec=0

 
# Configure the frame grabber 
setDefaultFrameGrabberChannel chan1
configureFrameGrabber chan=chan1
setOpticalCameraRotation angle=189
configureFrameGrabber flatfield=image
setOpticalCameraFov fov=11.5, chan=chan1
setOpticalCameraXImDir upright, chan=chan1
setOpticalCameraYImDir upright, chan=chan1
addSearchBox 8, 9, 501, 472, true, chan=chan1

configureFrameGrabber flatfield=image, chan=chan1
open grabber

# track the source, then do a big raster pattern.
track $src
until $acquired(source) | $elapsed > 1m

offset az=2, el=4:15

if($elevation($src)>15){

		mark add, f3
		grabFrame chan=chan1
		mark remove, f3
                until $elapsed>1m
        track $src
	until $acquired(source)
} else if ($elevation($src) < 15) {
	log "source too low"
}


