(Source src)

scan_catalog ~cbass/gcpCbass/control/scan/cbass_scan.cat

track $src
until $acquired(source)

mark add, f0
scan scanfile_charles1

until $elapsed > 12m
#until $acquired(scan)


mark add, f10
enableRxNoise(true)
until $elapsed >1m
enableRxNoise(false)
mark remove, f10

mark remove, f0
zeroScanOffsets
sky_offset x=0,y=0


