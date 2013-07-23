open grabber, dir=/mnt/data/cbass/fits
track jupiter
until $acquired(source)

do PointingOffset del=-2, 2, 0.15 {
	do PointingOffset daz=-2, 2, 0.15 {
		offset el=$del, az=$daz
		until $acquired(source)
		until $elapsed > 1s
		grabFrame chan=chan1
		until $elapsed > 1s

	} 
}

offset el=0, az=0
halt