import serial, time


def kill_camera():
	arduino.write('D')
        

if __name__ == '__main__':

    try:
		#arduino = serial.Serial("/dev/ttyACM0",9600)
		arduino = serial.Serial("/dev/ttyShutter",9600)
			
    except:
		print "Failed to connect"

    kill_camera()
    print "Camera is now OFF"
    time.sleep(5)
    arduino.close()
