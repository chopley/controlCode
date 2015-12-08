import serial, time


def kill_camera():
        time.sleep(2) #give the connection a second to settle
        arduino.write('D')


if __name__ == '__main__':

    try:
                #arduino = serial.Serial("/dev/ttyACM0",9600)                   
                arduino = serial.Serial("/dev/ttyShutter",115200,timeout=.2)

    except:
                print "Failed to connect"

    kill_camera()
    print "Camera is now OFF"
    time.sleep(5)
    arduino.close()


