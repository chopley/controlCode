(Source src)
# Do xy cross on source to find radio offsets

# Set archiving to the correct mode
archive combine=180, filter=true
open archive

# Clear all mark bits
mark remove, all

# We want model radio
model radio

# Make sure offsets are zero
offset az=0, el=0
sky_offset x=0, y=0
radec_offset ra=0, dec=0

# Track Source
track $src
until ($elapsed > 100s) |  ($acquired(source))		# wait for full stability

# Make sure atten is zero
setIFAtten input=0

# Scan in x

sky_offset x=-0.15, y=0
until $elapsed >30s
do PointingOffset dx= -0.15, 0.151, 0.075
{
  sky_offset x=$dx, y=0         # apply the offset
  #until $acquired(source)      # Should do this but it causes hang...
  until $elapsed > 30s		# wait for full stability
  mark one, f0	        	# request one marked frame 
  until $acquired(mark)		# wait for it to be written
}
# Scan in y
until $elapsed >30s
do PointingOffset dy= -0.15, 0.151, 0.075
{
  sky_offset x=0, y=$dy         # apply the offset
  until $elapsed > 30s		# wait for full stability
  mark one, f0	        	# request one marked frame 
  until $acquired(mark)		# wait for it to be written
}

  until $elapsed > 1m		# wait for full stability
close archive

sky_offset x=0, y=0         # zero offsets


