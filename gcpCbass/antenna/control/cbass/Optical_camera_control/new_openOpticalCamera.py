import serial, time

def open_shutter():
    time.sleep(2)
    arduino.write('O')
    time.sleep(2)

def check_shutter_is_open():
    time.sleep(2) #give the connection a second to settle
    print 'Sending request to Arduino'
    arduino.write('S')
    time.sleep(2)
    print 'Receiving reply from Arduino'
    data = arduino.read()
    if data:
        print 'Arduino received'
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
        print "WARNING: Shutter is still closed!"
    if data != 'O' and data != 'C':
        print "The shutter switch is not responding as expected"



if __name__ == '__main__':

    try:
                #arduino = serial.Serial("/dev/ttyACM0",115200)                   
                arduino = serial.Serial("/dev/ttyShutter",115200,timeout=.2)

    except:
                print "Failed to connect"

    open_shutter()
    print "Opening shutter..."
    time.sleep(10)
    check_shutter_is_open()
    arduino.close()

