import serial, time


def open_shutter():
	arduino.write('O')


def check_shutter_is_open():
	arduino.write('S')
	a = arduino.read()
        if a == 'O':
            print "Shutter is open"
        if a == 'C':
            print "WARNING: Shutter is still closed!"
        

if __name__ == '__main__':

    try:
		#arduino = serial.Serial("/dev/ttyACM0",9600)
		arduino = serial.Serial("/dev/ttyShutter",9600)
			
    except:
		print "Failed to connect"

    open_shutter()
    print "Opening shutter..."
    time.sleep(10)
    check_shutter_is_open()
    arduino.close()
