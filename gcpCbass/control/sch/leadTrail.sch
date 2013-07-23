(Source src, Scan scanName, Integer nreps, Time dtSlew)

#=======================================================================
# Slew to an RA offset by a given interval in time (dt), and scan
#=======================================================================

command offsetAndScan(Time ts, Time timeOffset) {

  #------------------------------------------------------------
  # Slew to an RA offset by the specified interval (dt), and 
  # wait until the telescope has acquired the new position
  #------------------------------------------------------------

  radec_offset dt=$timeOffset
  until $acquired(source)
  zeroScanOffsets  
  until $acquired(source)

  #------------------------------------------------------------
  # Now wait until the specified start time to start the scan
  #------------------------------------------------------------

  until $after($ts, utc)
  scan/add $scanName, nreps=$nreps
  until $acquired(scan)

}

#=======================================================================
# Main body of the schedule
#=======================================================================

Time ts = 00:00:00
Time tf = 00:00:00

#------------------------------------------------------------
# Compute the length of time it takes to scan
#------------------------------------------------------------

Time dtScan = $scan_len($scanName, nreps=$nreps)

#------------------------------------------------------------
# Calculate how long it takes to scan and slew to the new 
# position
#------------------------------------------------------------

Time dt = $dtScan + $dtSlew

PointingOffset dra = 0

#------------------------------------------------------------
# Track the source, making sure the RA offset is zeroed
#------------------------------------------------------------

radec_offset ra=0
track $src
until $acquired(source)

#------------------------------------------------------------
# Now that we are on-source, get the initial start time, 
# plus 5 seconds for setup.  This last is arbitrary, but just 
# ensures that we have a well-defined start time during which 
# the telescope is not slewing to acquire a source, etc.
#------------------------------------------------------------

ts = $time(utc) + 00:00:05

do Integer i=1,3,1 {

  #------------------------------------------------------------
  # Start scanning at the specified time
  #------------------------------------------------------------

  offsetAndScan $ts, 00:00:00
 
  #------------------------------------------------------------
  # Now increment by dt.  This will be the start time for the
  # trail field scan
  #------------------------------------------------------------

  ts = $ts + $dt

  #------------------------------------------------------------
  # Now offset by dt in RA and start the trail scan at the new
  # time ts
  #------------------------------------------------------------

  offsetAndScan $ts, $dt

  #------------------------------------------------------------
  # Now increment by dt.  This will be the start time for the
  # lead field scan
  #------------------------------------------------------------

  ts = $ts + $dt
}
