import serial, time

def check_shutter_is_closed():
	arduino.write('S')
	a = arduino.read()
        if a == 'O':
            print "Shutter is open...closing now"
            arduino.write('C')
        else:
            print "Shutter is closed"


if __name__ == '__main__':

    try:
		arduino = serial.Serial("/dev/ttyACM0",9600)
			
    except:
		print "Failed to connect"

    print "Checking shutter..."
    check_shutter_is_closed()
    time.sleep(5)
    arduino.close()
