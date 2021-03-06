(Time lststart, Time lststop)
# Cycle through list of radio sources taking a cross-scan on each

# sources must be really bright

import ~cbass/gcpCbass/control/sch/schedlib.sch

# Define source list, with an lst for each one.  This list is
# created by rad_set.m in order to maximize sky coverage.

group Source {
  Source name,    # source name
  Time  lst,      # lst to observe source
  Double flux     # source flux (in Jy)
}

listof Source sources = {
{J0225+621, 0.00, 10.00},
{    casa, 0.07, 10.00},
{J0225+621, 0.13, 10.00},
{    casa, 0.20, 10.00},
{    CygA, 0.27, 10.00},
{    casa, 0.33, 10.00},
{    CygA, 0.40, 10.00},
{    CygA, 0.47, 10.00},
{    casa, 0.53, 10.00},
{    CygA, 0.60, 10.00},
{J0225+621, 0.67, 10.00},
{J0225+621, 0.73, 10.00},
{    TauA, 0.80, 10.00},
{    CygA, 0.87, 10.00},
{    TauA, 0.93, 10.00},
{    casa, 1.00, 10.00},
{J0225+621, 1.07, 10.00},
{    CygA, 1.13, 10.00},
{    TauA, 1.20, 10.00},
{    CygA, 1.27, 10.00},
{    TauA, 1.33, 10.00},
{    CygA, 1.40, 10.00},
{    TauA, 1.47, 10.00},
{    CygA, 1.53, 10.00},
{J0225+621, 1.60, 10.00},
{    casa, 1.67, 10.00},
{    CygA, 1.73, 10.00},
{J0225+621, 1.80, 10.00},
{    casa, 1.87, 10.00},
{    TauA, 1.93, 10.00},
{    casa, 2.00, 10.00},
{    TauA, 2.07, 10.00},
{    TauA, 2.13, 10.00},
{    TauA, 2.20, 10.00},
{    casa, 2.27, 10.00},
{J0225+621, 2.33, 10.00},
{    TauA, 2.40, 10.00},
{    casa, 2.47, 10.00},
{    casa, 2.53, 10.00},
{J0225+621, 2.60, 10.00},
{    TauA, 2.67, 10.00},
{    TauA, 2.73, 10.00},
{    casa, 2.80, 10.00},
{J0225+621, 2.87, 10.00},
{    casa, 2.93, 10.00},
{    casa, 3.00, 10.00},
{J0225+621, 3.07, 10.00},
{    TauA, 3.13, 10.00},
{    casa, 3.20, 10.00},
{J0225+621, 3.27, 10.00},
{    casa, 3.33, 10.00},
{J0225+621, 3.40, 10.00},
{J0225+621, 3.47, 10.00},
{J0225+621, 3.53, 10.00},
{    TauA, 3.60, 10.00},
{J0225+621, 3.67, 10.00},
{    casa, 3.73, 10.00},
{    TauA, 3.80, 10.00},
{J0225+621, 3.87, 10.00},
{    casa, 3.93, 10.00},
{    TauA, 4.00, 10.00},
{    TauA, 4.07, 10.00},
{J0225+621, 4.13, 10.00},
{    casa, 4.20, 10.00},
{J0225+621, 4.27, 10.00},
{J0225+621, 4.33, 10.00},
{J0225+621, 4.40, 10.00},
{    TauA, 4.47, 10.00},
{    casa, 4.53, 10.00},
{    casa, 4.60, 10.00},
{J0225+621, 4.67, 10.00},
{    casa, 4.73, 10.00},
{    TauA, 4.80, 10.00},
{J0225+621, 4.87, 10.00},
{J0225+621, 4.93, 10.00},
{    TauA, 5.00, 10.00},
{    TauA, 5.07, 10.00},
{    casa, 5.13, 10.00},
{    casa, 5.20, 10.00},
{    casa, 5.27, 10.00},
{    casa, 5.33, 10.00},
{    TauA, 5.40, 10.00},
{J0225+621, 5.47, 10.00},
{    TauA, 5.53, 10.00},
{    casa, 5.60, 10.00},
{J0225+621, 5.67, 10.00},
{    casa, 5.73, 10.00},
{    casa, 5.80, 10.00},
{J0225+621, 5.87, 10.00},
{J0225+621, 5.93, 10.00},
{J0225+621, 6.00, 10.00},
{J0225+621, 6.07, 10.00},
{    casa, 6.13, 10.00},
{J0225+621, 6.20, 10.00},
{    TauA, 6.27, 10.00},
{J0225+621, 6.33, 10.00},
{    TauA, 6.40, 10.00},
{J0225+621, 6.47, 10.00},
{J0225+621, 6.53, 10.00},
{    TauA, 6.60, 10.00},
{J0225+621, 6.67, 10.00},
{J0225+621, 6.73, 10.00},
{    TauA, 6.80, 10.00},
{J0225+621, 6.87, 10.00},
{J0225+621, 6.93, 10.00},
{J0225+621, 7.00, 10.00},
{    TauA, 7.07, 10.00},
{J0225+621, 7.13, 10.00},
{    TauA, 7.20, 10.00},
{    TauA, 7.27, 10.00},
{    TauA, 7.33, 10.00},
{    TauA, 7.40, 10.00},
{J0225+621, 7.47, 10.00},
{J0225+621, 7.53, 10.00},
{J0225+621, 7.60, 10.00},
{J0225+621, 7.67, 10.00},
{J0225+621, 7.73, 10.00},
{    TauA, 7.80, 10.00},
{    TauA, 7.87, 10.00},
{    TauA, 7.93, 10.00},
{J0225+621, 8.00, 10.00},
{J1230+123, 8.07, 10.00},
{    TauA, 8.13, 10.00},
{J0225+621, 8.20, 10.00},
{J1230+123, 8.27, 10.00},
{J1230+123, 8.33, 10.00},
{    TauA, 8.40, 10.00},
{J0225+621, 8.47, 10.00},
{J1230+123, 8.53, 10.00},
{J0225+621, 8.60, 10.00},
{J1230+123, 8.67, 10.00},
{J1230+123, 8.73, 10.00},
{J1229+020, 8.80, 10.00},
{J0225+621, 8.87, 10.00},
{J0225+621, 8.93, 10.00},
{J0225+621, 9.00, 10.00},
{J0225+621, 9.07, 10.00},
{    TauA, 9.13, 10.00},
{J1230+123, 9.20, 10.00},
{    TauA, 9.27, 10.00},
{J1229+020, 9.33, 10.00},
{J1229+020, 9.40, 10.00},
{    TauA, 9.47, 10.00},
{J1230+123, 9.53, 10.00},
{J1230+123, 9.60, 10.00},
{J1229+020, 9.67, 10.00},
{J1229+020, 9.73, 10.00},
{    TauA, 9.80, 10.00},
{J1229+020, 9.87, 10.00},
{J1230+123, 9.93, 10.00},
{J1230+123, 10.00, 10.00},
{    TauA, 10.07, 10.00},
{    TauA, 10.13, 10.00},
{    TauA, 10.20, 10.00},
{J1230+123, 10.27, 10.00},
{    TauA, 10.33, 10.00},
{J1230+123, 10.40, 10.00},
{    TauA, 10.47, 10.00},
{J1230+123, 10.53, 10.00},
{J1229+020, 10.60, 10.00},
{J1229+020, 10.67, 10.00},
{J1229+020, 10.73, 10.00},
{J1230+123, 10.80, 10.00},
{J1230+123, 10.87, 10.00},
{J1229+020, 10.93, 10.00},
{J1229+020, 11.00, 10.00},
{J1229+020, 11.07, 10.00},
{J1230+123, 11.13, 10.00},
{J1229+020, 11.20, 10.00},
{J1229+020, 11.27, 10.00},
{J1230+123, 11.33, 10.00},
{J1229+020, 11.40, 10.00},
{J1229+020, 11.47, 10.00},
{J1229+020, 11.53, 10.00},
{J1229+020, 11.60, 10.00},
{J1229+020, 11.67, 10.00},
{J1229+020, 11.73, 10.00},
{J1230+123, 11.80, 10.00},
{J1229+020, 11.87, 10.00},
{J1229+020, 11.93, 10.00},
{J1229+020, 12.00, 10.00},
{J1229+020, 12.07, 10.00},
{J1230+123, 12.13, 10.00},
{J1230+123, 12.20, 10.00},
{J1230+123, 12.27, 10.00},
{J1230+123, 12.33, 10.00},
{J1230+123, 12.40, 10.00},
{J1230+123, 12.47, 10.00},
{J1230+123, 12.53, 10.00},
{J1230+123, 12.60, 10.00},
{J1229+020, 12.67, 10.00},
{J1230+123, 12.73, 10.00},
{J1229+020, 12.80, 10.00},
{J1230+123, 12.87, 10.00},
{J1229+020, 12.93, 10.00},
{J1229+020, 13.00, 10.00},
{J1229+020, 13.07, 10.00},
{J1229+020, 13.13, 10.00},
{J1229+020, 13.20, 10.00},
{J1229+020, 13.27, 10.00},
{J1230+123, 13.33, 10.00},
{J1230+123, 13.40, 10.00},
{J1230+123, 13.47, 10.00},
{J1230+123, 13.53, 10.00},
{J1229+020, 13.60, 10.00},
{J1229+020, 13.67, 10.00},
{J1229+020, 13.73, 10.00},
{J1230+123, 13.80, 10.00},
{J1230+123, 13.87, 10.00},
{J1229+020, 13.93, 10.00},
{J1229+020, 14.00, 10.00},
{J1230+123, 14.07, 10.00},
{J1229+020, 14.13, 10.00},
{J1230+123, 14.20, 10.00},
{J1229+020, 14.27, 10.00},
{J1229+020, 14.33, 10.00},
{J1230+123, 14.40, 10.00},
{    CygA, 14.47, 10.00},
{    CygA, 14.53, 10.00},
{    CygA, 14.60, 10.00},
{J1229+020, 14.67, 10.00},
{    CygA, 14.73, 10.00},
{    CygA, 14.80, 10.00},
{IR19207+1410, 14.87, 10.00},
{IR19207+1410, 14.93, 10.00},
{IR19207+1410, 15.00, 10.00},
{J1229+020, 15.07, 10.00},
{IR19207+1410, 15.13, 10.00},
{J1230+123, 15.20, 10.00},
{IR19207+1410, 15.27, 10.00},
{J1229+020, 15.33, 10.00},
{J1229+020, 15.40, 10.00},
{IR19207+1410, 15.47, 10.00},
{IR19207+1410, 15.53, 10.00},
{    CygA, 15.60, 10.00},
{J1230+123, 15.67, 10.00},
{    CygA, 15.73, 10.00},
{IR19207+1410, 15.80, 10.00},
{J1229+020, 15.87, 10.00},
{IR19207+1410, 15.93, 10.00},
{    CygA, 16.00, 10.00},
{IR19207+1410, 16.07, 10.00},
{J1230+123, 16.13, 10.00},
{J1229+020, 16.20, 10.00},
{    CygA, 16.27, 10.00},
{J1229+020, 16.33, 10.00},
{J1229+020, 16.40, 10.00},
{J1230+123, 16.47, 10.00},
{J1809-202, 16.53, 10.00},
{    casa, 16.60, 10.00},
{J1809-202, 16.67, 10.00},
{    CygA, 16.73, 10.00},
{J1809-202, 16.80, 10.00},
{    CygA, 16.87, 10.00},
{IR19207+1410, 16.93, 10.00},
{    CygA, 17.00, 10.00},
{    casa, 17.07, 10.00},
{J1809-202, 17.13, 10.00},
{    CygA, 17.20, 10.00},
{    casa, 17.27, 10.00},
{    casa, 17.33, 10.00},
{J1809-202, 17.40, 10.00},
{IR19207+1410, 17.47, 10.00},
{IR19207+1410, 17.53, 10.00},
{IR19207+1410, 17.60, 10.00},
{J1809-202, 17.67, 10.00},
{    casa, 17.73, 10.00},
{    casa, 17.80, 10.00},
{    CygA, 17.87, 10.00},
{    casa, 17.93, 10.00},
{IR19207+1410, 18.00, 10.00},
{J1809-202, 18.07, 10.00},
{IR19207+1410, 18.13, 10.00},
{    CygA, 18.20, 10.00},
{IR19207+1410, 18.27, 10.00},
{IR19207+1410, 18.33, 10.00},
{    CygA, 18.40, 10.00},
{    CygA, 18.47, 10.00},
{    CygA, 18.53, 10.00},
{    CygA, 18.60, 10.00},
{IR19207+1410, 18.67, 10.00},
{    casa, 18.73, 10.00},
{    casa, 18.80, 10.00},
{J1809-202, 18.87, 10.00},
{IR19207+1410, 18.93, 10.00},
{J1809-202, 19.00, 10.00},
{    CygA, 19.07, 10.00},
{    casa, 19.13, 10.00},
{    casa, 19.20, 10.00},
{IR19207+1410, 19.27, 10.00},
{J0225+621, 19.33, 10.00},
{    CygA, 19.40, 10.00},
{J1809-202, 19.47, 10.00},
{    CygA, 19.53, 10.00},
{    CygA, 19.60, 10.00},
{    casa, 19.67, 10.00},
{J0225+621, 19.73, 10.00},
{    CygA, 19.80, 10.00},
{IR19207+1410, 19.87, 10.00},
{J0225+621, 19.93, 10.00},
{J0225+621, 20.00, 10.00},
{    casa, 20.07, 10.00},
{IR19207+1410, 20.13, 10.00},
{    CygA, 20.20, 10.00},
{    CygA, 20.27, 10.00},
{J0225+621, 20.33, 10.00},
{    CygA, 20.40, 10.00},
{J0225+621, 20.47, 10.00},
{    CygA, 20.53, 10.00},
{IR19207+1410, 20.60, 10.00},
{IR19207+1410, 20.67, 10.00},
{    casa, 20.73, 10.00},
{J0225+621, 20.80, 10.00},
{    casa, 20.87, 10.00},
{    casa, 20.93, 10.00},
{    casa, 21.00, 10.00},
{J0225+621, 21.07, 10.00},
{    CygA, 21.13, 10.00},
{    casa, 21.20, 10.00},
{IR19207+1410, 21.27, 10.00},
{J0225+621, 21.33, 10.00},
{    CygA, 21.40, 10.00},
{J0225+621, 21.47, 10.00},
{IR19207+1410, 21.53, 10.00},
{J0225+621, 21.60, 10.00},
{    casa, 21.67, 10.00},
{    casa, 21.73, 10.00},
{    casa, 21.80, 10.00},
{IR19207+1410, 21.87, 10.00},
{IR19207+1410, 21.93, 10.00},
{J0225+621, 22.00, 10.00},
{    CygA, 22.07, 10.00},
{    CygA, 22.13, 10.00},
{J0225+621, 22.20, 10.00},
{J0225+621, 22.27, 10.00},
{IR19207+1410, 22.33, 10.00},
{IR19207+1410, 22.40, 10.00},
{    casa, 22.47, 10.00},
{    CygA, 22.53, 10.00},
{    CygA, 22.60, 10.00},
{    CygA, 22.67, 10.00},
{J0225+621, 22.73, 10.00},
{J0225+621, 22.80, 10.00},
{    casa, 22.87, 10.00},
{J0225+621, 22.93, 10.00},
{J0225+621, 23.00, 10.00},
{    CygA, 23.07, 10.00},
{IR19207+1410, 23.13, 10.00},
{    CygA, 23.20, 10.00},
{IR19207+1410, 23.27, 10.00},
{J0225+621, 23.33, 10.00},
{    casa, 23.40, 10.00},
{IR19207+1410, 23.47, 10.00},
{J0225+621, 23.53, 10.00},
{    casa, 23.60, 10.00},
{J0225+621, 23.67, 10.00},
{    CygA, 23.73, 10.00},
{    CygA, 23.80, 10.00},
{    casa, 23.87, 10.00},
{J0225+621, 23.93, 10.00}
}


# start observation
init_obs("Beginning schedule: radio_pointing.sch")

# do a sky dip
#do_sky_dip

# Set archiving to the correct mode
mark remove, all

#-----------------------------------------------------------------------
# Main loop -- track the next source,do a raster, move on
#-----------------------------------------------------------------------

log "Waiting for lststart..."
until $after($lststart,lst) 

  # Go through list
  foreach(Source source) $sources {

    # Break if past lststop
    if $time(lst)>$lststop {
       log "lststop reached"
       break
    }   

    # Skip sources whose lst has passed
    if $time(lst)>$source.lst  {
       log "too late for source",$source.name
    }

    # Do a cross on next available source
    if $time(lst)<$source.lst {

      if ($elevation($source.name) >= 27) {

      # Track Source
      until $after($source.lst,lst) # make sure we're not early
      log "Doing scan on ",$source.name

      # Do scan
      do_point_scan($source.name)


      zeroScanOffsets   	# zero offsets
    } #elevation requirement

    }
  }

# do a sky dip
#do_sky_dip

terminate("Finished schedule: radio_pointing.sch")

