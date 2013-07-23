(Source target)

# import the file we always need
import ~sptdaq/gcproot/control/sch/schedlib.sch

# always do at beginning of observation
init_obs ("starting spt_el_dip.sch")

# slew the telescope to where we want it to start
track $target
until $acquired(source)

# add the "spt_el_dip" bit
mark add, f7+f0
until $acquired(mark)

# 20d el scan at 0.1deg/sec
scan el-20d-0p1ds-1dss	
until $acquired(scan)

mark remove, f7+f0
until $acquired(mark)

# always do at end of observation
#terminate(finished "spt_el_dip.sch")

halt
