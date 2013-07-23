scan_catalog ~cbass/gcpCbass/control/scan/cbass_scan.cat

track sun
until $acquired(source)

scan big-sun

until $elapsed > 90m
#until $acquired(scan)

zeroScanOffsets


