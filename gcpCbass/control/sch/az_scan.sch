slew az=0, el=37

until $acquired(source)

scan_catalog ~cbass/gcpCbass/control/scan/cbass_scan.cat

scan az-4ds-360d-full-10reps-scan

until $elapsed > 30m
#until $acquired(scan)

zeroScanOffsets


