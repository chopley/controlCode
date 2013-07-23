(Source source, Time lststart, Time lststop) 
# number of points to record on each star, start time, stop time.


mark remove, all


archive combine=1, filter=false

# wait for lst start
until $after($lststart,lst)

	while($time(lst)>$lststop) {
		track $source
}
