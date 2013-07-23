(Source src, Interval time)

# schedule to integrate on a source for a while
import ~cbass/gcpCbass/control/sch/schedlib.sch

# initialize
init_obs("Beginning observe_source schedule")

# track the source, then do a big raster 
track $src
until $acquired(source)

# turn the noise source on
noise_on(5s)

# add feature
mark add, f12
until $elapsed>$time
mark remove, f12

# turn the noise source on
noise_on(5s)

# finish observation. 
terminate("done with integrating on source")
