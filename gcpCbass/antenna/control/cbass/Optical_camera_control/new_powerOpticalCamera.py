import serial, time


def power_camera():
	time.sleep(2) #give the connection a second to settle
        arduino.write('P')


if __name__ == '__main__':

    try:
                #arduino = serial.Serial("/dev/ttyACM0",9600)                   
                arduino = serial.Serial("/dev/ttyShutter",115200,timeout=0.2)

    except:
                print "Failed to connect"

    power_camera()
    print "Camera is now ON"
    time.sleep(5)
    arduino.close()

