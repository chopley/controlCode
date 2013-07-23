(Count npoints, PointingOffset azOffset, PointingOffset elOffset, Double snr) # number of points to record on each star

# gen set of stars using matlab function star_set in cbass_analysis archive

#should start roughly 22:30 in Lst, if numreps 8, should go for 2 hours.

# 45 sources * 8 reps * 3 reps/min ~ 2 hours,

listof Source sources = {
  MuHer,
  XiHer,
  NuHer,
  OmiHer,
  104Her,
  KapLyr,
  alpLyr,
  Zet1Lyr,
  Del2Lyr,
  GamLyr,
  Del2Lyr,
  EtaLyr,
  TheLyr,
  8Cyg,
  17Cyg,
  ChiCyg,
  EtaCyg,
  22Cyg,
  28Cyg,
  29Cyg,
  34Cyg,
  GamCyg,
  47Cyg,
  EpsCyg,
  52Cyg,
  31Vul,
  30Vul,
  29Vul,
  Gam2Del,
  DelDel,
  AlpDel,
  BetDel,
  Gam2Del,
  29Vul,
  30Vul,
  31Vul,
  52Cyg,
  EpsCyg,
  47Cyg,
  GamCyg,
  AlpCyg,
  55Cyg,
  59Cyg,
  63Cyg,
  XiCyg,
  68Cyg
}


# Get rid of any legacy signals.
signal/init done
mark remove, all

archive combine=1, filter=false
open grabber


model optical, ptel=0
offset az=$azOffset, el=$elOffset
sky_offset x=0, y=0
radec_offset ra=0, dec=0

 
# Configure the frame grabber 
setDefaultFrameGrabberChannel chan1
configureFrameGrabber chan=chan1
setOpticalCameraRotation angle=180
configureFrameGrabber flatfield=image
setOpticalCameraFov fov=11.5, chan=chan1
addSearchBox 11, 12, 495, 465, true, chan=chan1

# let's first take a flatfield
#slew az=180, el=80
#until $acquired(source)
#takeFlatfield chan=chan1
configureFrameGrabber flatfield=image, chan=chan1
configureFrameGrabber combine=10

foreach(Source s) $sources {

  if($elevation($s) > 20){

  track $s
  #remove offsets in case the lsat star was way off
  until $acquired(source)
  #wait for the antenna to settle
  until $elapsed > 5s

# Get an image
  grabFrame chan=chan1
  until $elapsed > 5s

# Check if we have a star within 10arcmin


  if($imstat(snr) > $snr & $peak(xabs) < 00:11:00 & $peak(yabs) < 00:11:00) {
     print "Found star at ",$peak(x),",",$peak(y)," with snr ",$imstat(snr)
     center # offset to star
     until $acquired(source)
     until $elapsed>5s # always wait for the telescope to settle after center

     # Now record a few points to sample the seeing disk
     do Count i=1,$npoints,1 {
	grabFrame chan=chan1
	until $elapsed>5s

	if($imstat(snr) > $snr & $peak(xabs) < 00:06:00 & $peak(yabs) < 00:06:00) {
	  print "Star offset ",$peak(x),",",$peak(y),", snr ",$imstat(snr)
	  center 
	  until $elapsed>5s
	  mark add, f0
	  until $elapsed>3s
	  mark remove, f0
	} else {
	  print "No star"
	}
     }
  } else {
    print "Failed to find star"
  }
  } #end elevation check

}

cleanup {
  offset az=0, el=0
  archive combine=1, filter=false
  model radio
}
