import serial, time

def check_status():
    arduino.write('S')	
    a = arduino.read()
    if a == 'O':
        print "Shutter is open"
    if a == 'C':
        print "Shutter is closed"


if __name__ == '__main__':

    try:
		#arduino = serial.Serial("/dev/ttyACM0",9600)
                arduino = serial.Serial("/dev/ttyShutter",9600)
			
    except:
		print "Failed to connect"

    check_status()
    arduino.close()





