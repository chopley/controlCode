#!/bin/awk -f


BEGIN {

  if(ARGC < 3) {
    printf("\nRead a SPT ephemeris file and output a delayed version of it\n\n")
    printf("\tUsage: input_filename delay=time (time in hours, < 24)\n\n")
    exit 1
  }

  for (i = 0; i < ARGC; i++) {
    if(ARGV[i] ~ /delay=/) {
      split(ARGV[i],sarr,"=")
      delete ARGV[i]
      delay = sarr[2]
      break
    }
  }
  split("Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec", months)
  split("31 28 31 30 31 30 31 31 30 31 30 31",days)
}

$3=="Ephemeris" {
    printf("#                     SPT Ephemeris for %s, delayed by %.2f hr\n",$5,delay)
}

$1 ~ /#/ && $3 != "Ephemeris" {
print $0
}

$1 !~ /#/ {
  mjd = $1
  year = substr($6,1,4)
  month = substr($6,6,3)
  day = substr($6,10,2)

  hour = substr($7,1,2)
  min = substr($7,4,2)

# Now figure out what month it is
 
  for(imonth=1;imonth <= 12;imonth++) {
    if(month==months[imonth])
      break
  }

# See if this is a leap year -- if it is, Feb has 29 days

  if(year % 4 == 0 && year % 100 != 0)
    days[2] = 29

# Now convert the time to hours

  time = hour + min/60

# And add the delay (in days) to the MJD 

  mjd += delay/24

# And add the delay (in hours) to the calendar date

  time += delay
 
# If the result is negative, we've gone back to the previous day

  if(time < 0) {
    time += 24
    day -= 1

# If the day is now 0, we've gone to the previous month

    if(day==0) {
      imonth -= 1

# If imonth is now 0, we've gone to the previous year

      if(imonth==0) {
        year -= 1
        imonth = 12
      }

# And get the day number appropriate for the end of this month

      day = days[imonth]
     }
  }

# Else if the result is positive, we've gone to the next day

  else if(time > 24) {
    time -= 24
    day += 1

# If the day is now > days[imonth], we've gone to the next month

    if(day > days[imonth]) {
      imonth += 1

# If imonth is now 13, we've gone to the next year

      if(imonth==13) {
        year += 1
        imonth = 1
      }

# And reset the day number

      day = 1
     }
    }

# Construct the new hour:min format out of the time

   hour = int(time)
   min = (time - hour)*60

# Increase the RA by the requested delay

   rah=substr($2,1,2)
   ram=substr($2,4,2)
   ras=substr($2,7,7)

   rah += delay

   if(rah>23) {
     rah -= 24
   }

   printf("%10.4f  %02d:%02d:%s  %s  %s  #  %4d-%s-%02d %02d:%02d\n",mjd,rah,ram,ras,$3,$4,year,months[imonth],day,hour,min)

}
END {}


