import serial, time


def power_camera():
	arduino.write('P')
        

if __name__ == '__main__':

    try:
		#arduino = serial.Serial("/dev/ttyACM0",9600)
		arduino = serial.Serial("/dev/ttyShutter",9600)
			
    except:
		print "Failed to connect"

    power_camera()
    print "Camera is now ON"
    time.sleep(5)
    arduino.close()
