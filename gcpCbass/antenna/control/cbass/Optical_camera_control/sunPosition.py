import ephem
import datetime

##Script that will estimate the Sun altitude  in thirty minutes. If the Sun is within 0.1 radians of the horizon at that time, we should close the optical camera and shut things down.
##CJC 18/1/2015

now = datetime.datetime.utcnow() #get current time
##now calculate the time in 30 minutes
UTCinThirtyMinutes = now+datetime.timedelta(0,0,0,0,30)
#ow = datetime.datetime.utcnow() #get current time
#ow = datetime.datetime(2015,01,18,17)
print('Time in UTC')
print(now)

Klerefontein=ephem.Observer()
Klerefontein.horizon='-5'
Klerefontein.lat='-31'
Klerefontein.lon='22'
##we use the time that it will be in thirty minutes to see if the Sun will be up?
Klerefontein.date=UTCinThirtyMinutes

sun=ephem.Sun()
next_set = (Klerefontein.next_setting(ephem.Sun()))
next_rise = (Klerefontein.next_rising(ephem.Sun()))
prev_set = (Klerefontein.previous_setting(ephem.Sun()))
prev_rise = (Klerefontein.previous_rising(ephem.Sun()))
now = Klerefontein.date

suntt=ephem.Sun(Klerefontein)
sunAltitude = suntt.alt

#angles are in radians. 0.1 radians is about 6 degrees- this is about how much the sun moves in half an hour. We are deciding based on whetther the sun will be within 0.1 radians of the horizon in thirty minutes so this should be ok.
if(sunAltitude>-0.1):
	print('Sun is up-- we should close the camera')
	print('First lets check the State of the optical camera Box---')
	execfile('checkOpticalCamera.py')
	execfile('killOpticalCamera.py')
	execfile('closeOpticalCamera.py')
else:
	print('Sun is down')
	print('Check the state of the Optical camera Box---')
	execfile('checkOpticalCamera.py')

