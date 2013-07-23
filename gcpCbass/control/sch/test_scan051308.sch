(Source target)

###################################################################
# az scans at various accelerations and speeds
###################################################################

# import the file we always need
import ~sptdaq/gcproot/control/sch/schedlib.sch

# always do at beginning of observation
init_obs("starting test_scan.sch")

track $target
until $acquired(source)

mark add, f0
until $acquired(mark)

#AZ scans

scan az-2p5d-0p25ds-0p1dss, nreps=2
until $acquired(scan)
until $elapsed>5s

scan az-2p5d-0p25ds-0p5dss, nreps=2
until $acquired(scan)
until $elapsed>5s

scan az-2p5d-0p25ds-1dss, nreps=2
until $acquired(scan)
until $elapsed>5s

scan az-2p5d-0p25ds-1p5dss, nreps=2
until $acquired(scan)
until $elapsed>5s

scan az-2p5d-0p25ds-2dss, nreps=2
until $acquired(scan)
until $elapsed>5s

scan az-2p5d-0p25ds-2p5dss, nreps=2
until $acquired(scan)
until $elapsed>5s

scan az-2p5d-0p25ds-3dss, nreps=2
until $acquired(scan)
until $elapsed>5s

scan az-2p5d-0p25ds-3p5dss, nreps=2
until $acquired(scan)
until $elapsed>5s

scan az-2p5d-0p25ds-4dss, nreps=2
until $acquired(scan)
until $elapsed>5s

mark remove, f0
until $acquired(mark)

# always do at end of observation
terminate("finished test_scan.sch")

halt
