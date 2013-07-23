# Do xy scans on Sun with collimation zero to get initial
# radio collimation estimates

# Avoid frying the PAM
setIFAtten input=17
# Adjust atten to get similar tp
setIFAtten input=25, ant=ant7
setIFAtten input=23, ant=ant4
setIFAtten input=22, ant=ant0
setIFAtten input=20, ant=ant2
setIFAtten input=20, ant=ant6
setIFAtten input=13, ant=ant3

# Need to be somewhere near or miss the Sun disc
sky_offset x=0, y=0
sky_offset y=-0.5, ant=3
sky_offset y=0.5, ant=0
sky_offset y=0.2, ant=5

# Clear all mark bits
mark remove, all

# We want model radio
model radio

# Set archiving to the correct mode
archive combine=1, filter=true
open archive

# Make sure other offsets are zero
offset az=0, el=0
radec_offset ra=0, dec=0

# Set radio collimation to zero as this is what we are trying to find
collimate radio, 0, 0

# Track sun
track sun
until $acquired(source)
until $elapsed > 30s		# wait for full stability

# Need to use offset/add as we already have some offsets
# set above to make scan hit Sun

# Loop over offsets in x dir
sky_offset/add x=-0.8           # go to scan start
do PointingOffset dx= -7, +7.1, 1
{
  sky_offset/add x=0.1          # step the scan
#  until $acquired(source)       # causes hang
  until $elapsed > 20s		# wait for full stability
  mark one, f0	        	# request one marked frame 
  until $acquired(mark)		# wait for it to be written
}
sky_offset/add x=-0.7           # apply initial offset

# Loop over offsets in y (el) dir
sky_offset/add y=-0.8           # go to scan start
do PointingOffset dy= -7, +7.1, 1
{
  sky_offset/add y=0.1          # step the scan
#  until $acquired(source)       # causes hang
  until $elapsed > 20s		# wait for full stability
  mark one, f0	        	# request one marked frame 
  until $acquired(mark)		# wait for it to be written
 }
sky_offset/add y=-0.7

close archive

archive combine=20, filter=false

# Remember to unset atten after slewing off Sun!
