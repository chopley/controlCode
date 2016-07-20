(PointingOffset elOffset, Integer scanOrder)

# schedule of cbass survey schedule
# does a full scan in azimuth, does noise source
import ~cbassuser/cbass/gcpCbass/control/sch/schedlibSouth.sch

import ~cbassuser/cbass/gcpCbass/control/sch/scanlibSouth.sch
encoder_limits  -280, 145, 15, 84, 0, 0  

## define the list of scans
#group Scanlist {
#    Scan name_10sdead,  #scan name 10s dead
#    Scan name_nodead,    #scan name with no dead
#    Double flux
#}
#
#print "scan order", $scanOrder
#
# Default list
#listof Scanlist scansThisSched = {
#  {survey-3.8ds-10sdead, survey-3.8ds-nodead, 10},
#  {survey-3.9ds-10sdead, survey-3.9ds-nodead, 10},
#  {survey-4.0ds-10sdead, survey-4.0ds-nodead, 10},
#  {survey-4.1ds-10sdead, survey-4.1ds-nodead, 10},
#  {survey-4.2ds-10sdead, survey-4.2ds-nodead, 10}
#}
#
#
#
## define the order in which we're doing scans
#if($scanOrder==1){
#  listof Scanlist scansThisSched = {
#    {survey-3.8ds-10sdead, survey-3.8ds-nodead, 10},
#    {survey-3.9ds-10sdead, survey-3.9ds-nodead, 10},
#    {survey-4.0ds-10sdead, survey-4.0ds-nodead, 10},
#    {survey-4.1ds-10sdead, survey-4.1ds-nodead, 10},
#    {survey-4.2ds-10sdead, survey-4.2ds-nodead, 10}
#  }
#} else if($scanOrder==2) {
#  listof Scanlist scansThisSched = {
#    {survey-3.9ds-10sdead, survey-3.9ds-nodead, 10},
#    {survey-4.0ds-10sdead, survey-4.0ds-nodead, 10},
#    {survey-4.1ds-10sdead, survey-4.1ds-nodead, 10},
#    {survey-4.2ds-10sdead, survey-4.2ds-nodead, 10},
#    {survey-3.8ds-10sdead, survey-3.8ds-nodead, 10}
#  }
#} else if($scanOrder==3) {
#  print "made it here"
#  listof Scanlist scansThisSched = {
#    {survey-4.0ds-10sdead, survey-4.0ds-nodead, 10},
#    {survey-4.1ds-10sdead, survey-4.1ds-nodead, 10},
#    {survey-4.2ds-10sdead, survey-4.2ds-nodead, 10},
#    {survey-3.8ds-10sdead, survey-3.8ds-nodead, 10},
#    {survey-3.9ds-10sdead, survey-3.9ds-nodead, 10}
#  }
#
#foreach (Scanlist scanlist) $scansThisSched { 
#	print "this scan", $scanlist.name_10sdead
#}
#
#
#} else if($scanOrder==4) {
#  print "made it here 4"
#  listof Scanlist scansThisSched = {
#    {survey-4.1ds-10sdead, survey-4.1ds-nodead, 10},
#    {survey-4.2ds-10sdead, survey-4.2ds-nodead, 10},
#    {survey-3.8ds-10sdead, survey-3.8ds-nodead, 10},
#    {survey-3.9ds-10sdead, survey-3.9ds-nodead, 10},
#    {survey-4.0ds-10sdead, survey-4.0ds-nodead, 10}
#  }
#} else if($scanOrder==5) {
#  listof Scanlist scansThisSched = {
#    {survey-4.2ds-10sdead, survey-4.2ds-nodead, 10},
#    {survey-3.8ds-10sdead, survey-3.8ds-nodead, 10},
#    {survey-3.9ds-10sdead, survey-3.9ds-nodead, 10},
#    {survey-4.0ds-10sdead, survey-4.0ds-nodead, 10},
#    {survey-4.1ds-10sdead, survey-4.1ds-nodead, 10}
#  }
#}

# initialize
init_obs("beginning of cbass survey schedule")

setCryoSkyTemp 1,13
# slew to the start location.
slew az=0,el=30
until $acquired(source)|$elapsed>3m
slew az=-100
until $acquired(source)|$elapsed>1m
# track NCP
track scpfixed
until $acquired(source)|$elapsed>1m
offset el=$elOffset
slew az=-240
until $acquired(source)|$elapsed>1m
until $elapsed>10s

# Some preliminary calibration data:
#
# Noise!
noise_on_general 5s, 8s

if($scanOrder==1) {
  observeScanList1
} else if ($scanOrder==2) {
  observeScanList2
} else if ($scanOrder==3) {
  observeScanList3
} else if ($scanOrder==4) {
  observeScanList4
} else if ($scanOrder==5) {
  observeScanList5
}

# Wrap things up.

# Return to az=0.  This doesn't affect the elevation offset.
zeroScanOffsets
track scpfixed
until $acquired(source)|$elapsed>3m	

# Use the noise source.
noise_on_general 5s,8s

# Sky dip.
do_large_sky_dip

# Return to start position.
zeroScanOffsets
track scpfixed
until $acquired(source)|$elapsed>1m

# And finish up.
terminate("done with cbass survey schedule")
