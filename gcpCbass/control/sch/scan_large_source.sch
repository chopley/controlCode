(Source src)

scan_catalog ~cbass/gcpCbass/control/scan/cbass_scan.cat

track $src
until $acquired(source)

scan large-source-scan

until $elapsed > 45m
#until $acquired(scan)

zeroScanOffsets


