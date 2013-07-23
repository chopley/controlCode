( Source src, Time lststart, Time lststop)

# wait for lst start
until $after($lststart,lst)

# track the source, wait for it to be tracking
track $src
until $acquired(source)


# until the lst stop, just take an image of the source.
while($time(lst)<$lststop){

	sky_offset x=0, y=0
	until $acquired(source)
	mark add, f0
	until $elapsed>3m
	mark remove, f0

	sky_offset x=5, y=5
	until $acquired(source)
	mark add, f1
	until $elapsed > 3m
	mark remove, f1

}
	







