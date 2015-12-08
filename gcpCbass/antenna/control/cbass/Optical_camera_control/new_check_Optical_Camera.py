import serial,time

def check_status():
    time.sleep(2) #give the connection a second to settle
    print 'Sending request to Arduino'
    arduino.write('S')
    time.sleep(2)
    print 'Receiving reply from Arduino'
    data = arduino.read()
    if data:
        print 'Arduino reply received'
        #print data
    else:
        flag = 0
        print 'I have failed to read data'
        while flag < 10:
                data = arduino.read()
                print 'Still failing'
                flag += 1
                if data:
                        print 'I have finally read something'
                        #print data
    if data == 'O':
        print "Shutter is open"
    if data == 'C':
        print "Shutter is closed"
    if data != 'O' and data != 'C':
        print "The shutter switch is not responding as expected"





if __name__ == '__main__':

    try:
                #arduino = serial.Serial("/dev/ttyACM0",115200,timeout=.2)       
                arduino = serial.Serial("/dev/ttyShutter",115200,timeout=.2)

    except:
                print "Failed to connect"

    check_status()
    arduino.close()
