command observeScanList1()
{

  # define the list of scans
  group Scanlist {
      Scan name_10sdead,  #scan name 10s dead
      Scan name_nodead,    #scan name with no dead
      Double flux
  }
  
  listof Scanlist scansThisSched = {
    {survey-3.8ds-10sdead, survey-3.8ds-nodead, 10},
    {survey-3.9ds-10sdead, survey-3.9ds-nodead, 10},
    {survey-4.0ds-10sdead, survey-4.0ds-nodead, 10},
    {survey-4.1ds-10sdead, survey-4.1ds-nodead, 10},
    {survey-4.2ds-10sdead, survey-4.2ds-nodead, 10}
  }


  # Return to az=0.  This doesn't affect the elevation offset.
  zeroScanOffsets
  slew az=-250
  until $acquired(source)|$elapsed>60s

  # Use the noise source.
  noise_on_general 5s,8s

  # Sky dip.
  do_large_sky_dip

   # Return to start position.
  foreach (Scanlist scanlist) $scansThisSched {


    zeroScanOffsets
    zeroScanOffsets
  slew az=-250
  until $acquired(source)|$elapsed>60s

    Boolean start = true
    do Count i=1,5,1{

      # Fire the noise diode.
      noise_on_general 5s,8s
       if ($start) {
          # do the scan with 10s dead time
          mark add, f0
          scan $scanlist.name_10sdead
          start = false
       } else {
          # do scan without dead time
	  mark add, f0
          scan $scanlist.name_nodead
       }
       until $acquired(scan)|$elapsed>4m
       mark remove, f0
    }
  }
}

#------------------------------------------------------------
command observeScanList2()
{

  # define the list of scans
  group Scanlist {
      Scan name_10sdead,  #scan name 10s dead
      Scan name_nodead,    #scan name with no dead
      Double flux
  }
  
  listof Scanlist scansThisSched = {
    {survey-3.9ds-10sdead, survey-3.9ds-nodead, 10},
    {survey-4.0ds-10sdead, survey-4.0ds-nodead, 10},
    {survey-4.1ds-10sdead, survey-4.1ds-nodead, 10},
    {survey-4.2ds-10sdead, survey-4.2ds-nodead, 10},
    {survey-3.8ds-10sdead, survey-3.8ds-nodead, 10}
  }
  
  # Return to az=0.  This doesn't affect the elevation offset.
  zeroScanOffsets
  slew az=-250
  until $acquired(source)|$elapsed>60s
  

  # Use the noise source.
  noise_on_general 5s,8s

  # Sky dip.
  do_large_sky_dip

   # Return to start position.
  foreach (Scanlist scanlist) $scansThisSched {


    zeroScanOffsets
  slew az=-250
  until $acquired(source)|$elapsed>60s

    Boolean start = true
    do Count i=1,5,1{

      # Fire the noise diode.
      noise_on_general 5s,8s
       if ($start) {
          # do the scan with 10s dead time
          mark add, f0
          scan $scanlist.name_10sdead
          start = false
       } else {
          # do scan without dead time
	  mark add, f0
          scan $scanlist.name_nodead
       }
       until $acquired(scan)|$elapsed>4m
       mark remove, f0
    }
  }
}



#------------------------------------------------------------  
command observeScanList3()
{
  
  # define the list of scans
  group Scanlist {
      Scan name_10sdead,  #scan name 10s dead
      Scan name_nodead,    #scan name with no dead
      Double flux
  }
  listof Scanlist scansThisSched = {
    {survey-4.0ds-10sdead, survey-4.0ds-nodead, 10},
    {survey-4.1ds-10sdead, survey-4.1ds-nodead, 10},
    {survey-4.2ds-10sdead, survey-4.2ds-nodead, 10},
    {survey-3.8ds-10sdead, survey-3.8ds-nodead, 10},
    {survey-3.9ds-10sdead, survey-3.9ds-nodead, 10}
  }
  # Return to az=0.  This doesn't affect the elevation offset.
  zeroScanOffsets
  slew az=-250
  until $acquired(source)|$elapsed>60s

  # Use the noise source.
  noise_on_general 5s,8s

  # Sky dip.
  do_large_sky_dip

   # Return to start position.
  foreach (Scanlist scanlist) $scansThisSched {


    zeroScanOffsets
  slew az=-250
  until $acquired(source)|$elapsed>60s

    Boolean start = true
    do Count i=1,5,1{

      # Fire the noise diode.
      noise_on_general 5s,8s
       if ($start) {
          # do the scan with 10s dead time
          mark add, f0
          scan $scanlist.name_10sdead
          start = false
       } else {
          # do scan without dead time
	  mark add, f0
          scan $scanlist.name_nodead
       }
       until $acquired(scan)|$elapsed>4m
       mark remove, f0
    }
  }
}

  

#------------------------------------------------------------  
command observeScanList4()
{
  
  # define the list of scans
  group Scanlist {
      Scan name_10sdead,  #scan name 10s dead
      Scan name_nodead,    #scan name with no dead
      Double flux
  }
  listof Scanlist scansThisSched = {
    {survey-4.1ds-10sdead, survey-4.1ds-nodead, 10},
    {survey-4.2ds-10sdead, survey-4.2ds-nodead, 10},
    {survey-3.8ds-10sdead, survey-3.8ds-nodead, 10},
    {survey-3.9ds-10sdead, survey-3.9ds-nodead, 10},
    {survey-4.0ds-10sdead, survey-4.0ds-nodead, 10}
  }
  
  # Return to az=0.  This doesn't affect the elevation offset.
  zeroScanOffsets
  slew az=-250
  until $acquired(source)|$elapsed>60s

  # Use the noise source.
  noise_on_general 5s,8s

  # Sky dip.
  do_large_sky_dip

   # Return to start position.
  foreach (Scanlist scanlist) $scansThisSched {


    zeroScanOffsets
    zeroScanOffsets
  slew az=-250
  until $acquired(source)|$elapsed>60s

    Boolean start = true
    do Count i=1,5,1{

      # Fire the noise diode.
      noise_on_general 5s,8s
       if ($start) {
          # do the scan with 10s dead time
          mark add, f0
          scan $scanlist.name_10sdead
          start = false
       } else {
          # do scan without dead time
	  mark add, f0
          scan $scanlist.name_nodead
       }
       until $acquired(scan)|$elapsed>4m
       mark remove, f0
    }
  }
}



#------------------------------------------------------------  
command observeScanList5()
{
  
  # define the list of scans
  group Scanlist {
      Scan name_10sdead,  #scan name 10s dead
      Scan name_nodead,    #scan name with no dead
      Double flux
  }
  listof Scanlist scansThisSched = {
    {survey-4.2ds-10sdead, survey-4.2ds-nodead, 10},
    {survey-3.8ds-10sdead, survey-3.8ds-nodead, 10},
    {survey-3.9ds-10sdead, survey-3.9ds-nodead, 10},
    {survey-4.0ds-10sdead, survey-4.0ds-nodead, 10},
    {survey-4.1ds-10sdead, survey-4.1ds-nodead, 10}
  }

  # Return to az=0.  This doesn't affect the elevation offset.
  zeroScanOffsets
  slew az=-250
  until $acquired(source)|$elapsed>60s

  # Use the noise source.
  noise_on_general 5s,8s

  # Sky dip.
  do_large_sky_dip

   # Return to start position.
  foreach (Scanlist scanlist) $scansThisSched {


    zeroScanOffsets
    zeroScanOffsets
  slew az=-250
  until $acquired(source)|$elapsed>60s

    Boolean start = true
    do Count i=1,5,1{

      # Fire the noise diode.
      noise_on_general 5s,8s
       if ($start) {
          # do the scan with 10s dead time
          mark add, f0
          scan $scanlist.name_10sdead
          start = false
       } else {
          # do scan without dead time
	  mark add, f0
          scan $scanlist.name_nodead
       }
       until $acquired(scan)|$elapsed>4m
       mark remove, f0
    }
  }
}



#------------------------------------------------------------
