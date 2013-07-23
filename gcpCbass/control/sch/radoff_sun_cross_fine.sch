# Do close spaced scans on Sun

# Avoid overloading PAM
setIFAtten input=17
# Adjust atten to get similar tp
setIFAtten input=25, ant=ant7
setIFAtten input=23, ant=ant4
setIFAtten input=22, ant=ant0
setIFAtten input=20, ant=ant2
setIFAtten input=20, ant=ant6
setIFAtten input=13, ant=ant3

# Clear all mark bits
mark remove, all

# We want model radio
model radio

# Set archiving to the correct mode
archive combine=1, filter=true
open archive

# Make sure offsets are zero
offset az=0, el=0
sky_offset x=0, y=0
radec_offset ra=0, dec=0

# Track sun
track sun
until $acquired(source)
until $elapsed > 30s		# wait for full stability

sky_offset/add x=0.525         # go to x scan start
until $elapsed > 30s		# wait for full stability

# Scan in x
do PointingOffset dx= -0.5, 0.5001, 0.025
{
  sky_offset x=$dx, y=0         # step the scan
#  until $acquired(source)       # causes hang!  
  until $elapsed > 15s		# wait for full stability
  mark one, f0	        	# request one marked frame 
  until $acquired(mark)		# wait for it to be written
}

# Scan in y
do PointingOffset dy= -0.5, 0.5001, 0.025
{
  sky_offset x=0, y=$dy         # step the scan
#  until $acquired(source)       # causes hang!  
  until $elapsed > 15s		# wait for full stability
  mark one, f0	        	# request one marked frame 
  until $acquired(mark)		# wait for it to be written
}

# Remember to unset atten after slewing off Sun!
close archive

sky_offset/add x=0, y=0         # reset to zero

archive combine=20, filter=false
