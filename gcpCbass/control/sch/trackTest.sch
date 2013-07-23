( Source src, Time lststart, Time lststop)

# Get rid of any legacy signals.
signal/init done
mark remove, all

# Archive only marked frames
archive combine=1, filter=true

model optical, ptel=0
#offset az=0, el=0
#sky_offset x=0, y=0
#radec_offset ra=0, dec=0

# Configure the frame grabber
setDefaultFrameGrabberChannel chan1
configureFrameGrabber chan=chan1
setOpticalCameraRotation angle=3:30:00
configureFrameGrabber flatfield=image
setOpticalCameraFov fov=12, chan=chan1
addSearchBox 14, 17, 501, 462, true, chan=chan1
addSearchBox 293, 227, 302, 242, false, chan=chan1
addSearchBox 299, 433, 310, 449, false, chan=chan1
addSearchBox 188, 380, 204, 400, false, chan=chan1

# let's first take a flatfield
#slew az=180, el=80
#until $acquired(source)
#takeFlatfield chan=chan1
configureFrameGrabber flatfield=image, chan=chan1
configureFrameGrabber combine=1
open grabber, dir="/mnt/data/cbass/fits"




# wait for lst start
until $after($lststart,lst)

# track the source, wait for it to be tracking
track $src
until $acquired(source)
until $elapsed > 3s




# until the lst stop, just take an image of the source.
while($time(lst)<$lststop){
	# should just do it until we're done.
	grabFrame chan=chan1
}
	







