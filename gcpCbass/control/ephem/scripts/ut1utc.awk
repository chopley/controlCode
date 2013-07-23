#!/bin/awk -f


BEGIN {

# When the expected column headings of the table have been seen, in_table
# will be set to 1.

  in_table = 0

# Create a lookup table of 3-letter month names.

  split("Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec", months)
}

# In the current bulletin there are four column headings. The first
# three columns don't have names. Once these headings have been seen
# the file header has passed and the table of data should start.

$1=="MJD" && $2=="x(arcsec)" && $3=="y(arcsec)" && $4=="UT1-UTC(sec)" {
  in_table = 1;

# Describe the contents of the ut1-utc ephemeris that is being produced.

  printf("# CBI Ephemeris of UT1-UTC\n");
  printf("#\n");
  printf("# Note that MJD = JD - 2400000.5\n");
  printf("#\n");
  printf("# UTC (MJD)  UT1-UTC (s)  Calendar (UTC)\n");
  printf("#----------  -----------  --------------\n");

}

# The current bulletin has 7 columns, which are:
#  year, month(1-12), day(1-31), MJD x(arcsec) y(arcsec) UT1-UTC(sec).

in_table!=0 && NF==7 && $1 ~ /^(19|20)[0-9][0-9]/ {
  year = $1;
  month = months[$2];
  day = $3;
  mjd = $4;
  ut1utc = $7;
  printf("%.5f  %+10.6f   # %s-%s-%02d\n", mjd, ut1utc, year, month, day);
}

