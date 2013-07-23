#!/bin/awk -f
#-----------------------------------------------------------------------
# Convert the format of an E-mailed JPL ephemeris to a machine readable
# form. To have a new ephemeris sent by JPL, change the COMMAND, START_TIME,
# STOP_TIME and STEP_SIZE parameters of the following, remove the initial
# # characters and E-mail the result to horizons@ssd.jpl.nasa.gov, along
# with the subject line: JOB-SUBMITTAL
#
#!$$SOF (ssd)         JPL/Horizons Execution Control VARLIST          July, 1997
# EMAIL_ADDR =  mcs@astro.caltech.edu
# COMMAND    = 'mars'
# OBJ_DATA   = NO
# MAKE_EPHEM = YES 
# TABLE_TYPE = OBS 
# CENTER     = GEO
# COORD_TYPE = GEODETIC
# SITE_COORD = '0,0,0'
# START_TIME = '1997-JAN-1 00:00'
# STOP_TIME  = '1998-JAN-1 00:00'
# STEP_SIZE  = '1 days'
# QUANTITIES = '2,20'
# REF_SYSTEM = J2000 
# CAL_FORMAT = BOTH
# ANG_FORMAT = HMS
# APPARENT   = AIRLESS
# TIME_TYPE  = TT
# ELEV_CUT   = -90
# SKIP_DAYLT = NO
# AIRMASS    = 38.D0 
# EXTRA_PREC = YES
# CSV_FORMAT = NO
#!$$EOF+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#
#-----------------------------------------------------------------------

# The following variable will be 1 until the start of the ephemeris
# is seen. This allows for rejection of lines of the header section
# that might otherwise be interpreted as ephemeris entries.

BEGIN {
  in_header=1
}


# Get the name of the target planet from the header.

/^Target body/ {
  printf("#                     SPT Ephemeris for %s\n", $4);
}


# The following pattern matches the start of the ephemeris table.

/^\$\$SOE/ {

# Signal that we have finished reading the header section.

  in_header=0;

# Describe the contents of the file.

  printf("#\n");
  printf("# Note that TT ~= TAI + 32.184.\n");
  printf("# See slaDtt.c for how to convert TT to UTC.\n");
  printf("#\n");
  printf("# Also note that MJD = JD - 2400000.5\n");
  printf("#\n");
  printf("# MJD (TT)   Right Ascen    Declination   Distance (au)      Calendar (TT)\n");
  printf("#---------  -------------  -------------  ------------     -----------------\n");
}


# Output reformatted ephemeris lines.

/^ [0-9]/ && !in_header {
    printf("%.4f  %s:%s:%s  %s:%s:%s  %s  #  %s %s\n",
	   $3 - 2400000.5, $4,$5,$6, $7,$8,$9, $10, $1, $2);
}


# The job is complete when the end of the ephemeris table is reached.

/^\$\$EOE/ {
  exit
}

