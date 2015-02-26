import serial, time


def close_shutter():
	arduino.write('C')


def check_shutter_is_closed():
	arduino.write('S')
	a = arduino.read()
        if a == 'O':
            print "WARNING: Shutter is still open!"
        if a == 'C':
            print "Shutter is closed"
        

if __name__ == '__main__':

    try:
		#arduino = serial.Serial("/dev/ttyACM0",9600)
		arduino = serial.Serial("/dev/ttyShutter",9600)
			
    except:
		print "Failed to connect"

    close_shutter()
    print "Closing shutter..."
    time.sleep(5)
    check_shutter_is_closed()
    arduino.close()
