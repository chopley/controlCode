(Count npoints, PointingOffset azOffset, PointingOffset elOffset, Double snr) # number of points to record on each star

# gen set of stars using matlab function star_set in cbass_analysis archive

# 62 stars * x times * /3 times/min = 
# should start around lst of 21.  go for about 4 hours.
# moves in az from ~120 to 280

listof Source sources = {
  LamPsc,
  IotPsc,
  ThePsc,
  GamPsc,
  BetPsc,
  55Peg,
  RhoPeg,
  XiPeg,
  ZetPeg,
  XiPeg,
  AlpPeg,
  UpsPeg,
  78Peg,
  AlpAnd,
  EpsAnd,
  DelAnd,
  PiAnd,    #17
  MuAnd,
  NuAnd,
  PiCas,
  XiCas,
  ZetCas,
  AlpCas,
  BetCas,
  4Cas,
  IotCep,
  XiCep,
  9Cep,
  AlpCep,
  EtaCep,
  TheCep,  #31
  EtaCep,
  33Cyg,
  PsiCyg,
  TheCyg,
  DelCyg,
  22Cyg,
  28Cyg,
  34Cyg,
  GamCyg,
  29Cyg,
  22Cyg,
  EtaCyg,
  ChiCyg,
  17Cyg,
  PhiCyg,
  Bet1Cyg,
  AlpVul,
  9Vul,  #49
  ZetSge,
  12Vul,
  15Vul,
  23Vul,
  41Cyg,
  39Cyg,
  47Cyg,
  EpsCyg,
  LamCyg,
  NuCyg,
  XiCyg,
  68Cyg,
  SigCyg,
  72Cyg  #62
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
