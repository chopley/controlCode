open grabber
track moon
until $elapsed > 120s

do PointingOffset del=-2, 2.01, 0.15 {
	do PointingOffset daz=-2, 2.01, 0.15 {
		offset el=$del, az=$daz
		until $elapsed > 5s
		grabFrame chan=chan1	
		until $elapsed > 3s
	}
}

offset el=0, az=0
halt