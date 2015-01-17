#!/usr/bin/env python
'''
This script allows testing of the phase switch modulation and demodulation on C-BASS- inject a CW source (preferably quite low as it's easier to see the phase!) around 4520MHz into the C-BASS input port. Run this script and you will get two output files ADCSamplesNeg.txt and ADCSamplesPos.txt. These will show you the actual ADC values that are being presented to the FFT after the demodulation . You can adjust the actual position of the demodulation and the blanking window- just look at the appropriate registers. Note that the design begins with demodulation- so to turn it off you have to write a 1 to a register.
'''

#TODO: add support for ADC histogram plotting.
#TODO: add support for determining ADC input level 

import corr,time,numpy,struct,sys,logging,pylab,serial

bitstreamprelim = 'cbassrx_phaseswitch19apr_2012_Apr_19_2004'
bitstream = bitstreamprelim+'.bof'
katcp_port=7147
sleep_time=0.005
def exit_fail():
    print 'FAILURE DETECTED. Log entries:\n',lh.printMessages()
    try:
        fpga.stop()
    except: pass
    raise
    exit()

def exit_clean():
    try:
        fpga.stop()
    except: pass
    exit()


def diodeOn():
    fpga.write_int('gpio_set',0)
    fpga.write_int('gpio1_set',1)
    return 1

def diodeOff():
    fpga.write_int('gpio_set',1)
    fpga.write_int('gpio1_set',0)
    return 0

def diodeState(state):
    if(state==1):#turn diode on
    	fpga.write_int('gpio_set',0)
    	fpga.write_int('gpio1_set',1)
    elif(state==0):#turn diode off
    	fpga.write_int('gpio_set',1)
    	fpga.write_int('gpio1_set',0)
    return state	
    	

def bramw(fpga, bname, odata, samples=1024):
    b_0=struct.pack('>'+str(samples)+'i',*odata)
    fpga.write(bname,b_0)
    return

def getSnapCat(snap=""):
    #print 'into getSnapBlock'
    #print snap
    snapctrl=snap+'_ctrl'
    snapaddr=snap+'_addr'
    snapbram=snap+'_bram'

    fpga.write_int(snapctrl,4)	
    fpga.write_int(snapctrl,5)	
    a=fpga.read_uint(snapaddr)
    while(a<2047):
        a=fpga.read_uint(snapaddr)
	print a
	time.sleep(sleep_time)
#	print 'waiting'
    snapreturn=struct.unpack('>8192b',fpga.read(snapbram,2048*4,0))

    return snapreturn

def getSnap(snap=""):
    #print 'into getSnapBlock'
    #print snap
    snapctrl=snap+'_ctrl'
    snapaddr=snap+'_addr'
    snapbram=snap+'_bram'

    fpga.write_int(snapctrl,4)	
    fpga.write_int(snapctrl,5)	
    a=fpga.read_uint(snapaddr)
    while(a<2047):
        a=fpga.read_uint(snapaddr)
	print a
	time.sleep(sleep_time)
#	print 'waiting'
    snapreturn=struct.unpack('>2048I',fpga.read(snapbram,2048*4,0))

    return snapreturn

def getSnapBlock(snap=""):
    #print 'into getSnapBlock'
    #print snap
    snapctrl='fft_'+snap+'_ctrl'
    snapaddr='fft_'+snap+'_addr'
    snapbram='fft_'+snap+'_bram'
    #fpga.write_int(snapctrl,6)
    #fpga.write_int(snapctrl,5)
    #fpga.write_int(snapctrl,7)	

    fpga.write_int(snapctrl,0)	
    fpga.write_int(snapctrl,1)	
    while(fpga.read_uint(snapaddr)<100):
	time.sleep(sleep_time)
#	print 'waiting'
    snapreturn=struct.unpack(fftstringUnsigned,fpga.read(snapbram,fftreal*4,0))
    time.sleep(sleep_time)

    return snapreturn

def getADCSnapBlock(snap=""):
    #print 'into getSnapBlock'
    #print snap
    snapctrl='fft_'+snap+'_ctrl'
    snapaddr='fft_'+snap+'_addr'
    snapbram='fft_'+snap+'_bram'
    fpga.write_int(snapctrl,0)
    fpga.write_int(snapctrl,5)
    fpga.write_int(snapctrl,7)	
    while(fpga.read_uint(snapaddr)<100):
	time.sleep(sleep_time)
#	print 'waiting'
    snapreturn=struct.unpack(ADCstringSigned,fpga.read(snapbram,256,0))
    time.sleep(sleep_time)
    return snapreturn

#START OF MAIN:

if __name__ == '__main__':
    from optparse import OptionParser

    p = OptionParser()
    p.set_usage('spectrometer.py <ROACH_HOSTNAME_or_IP> [options]')
    p.set_description(__doc__)
    p.add_option('-l', '--acc_len', dest='acc_len', type='int',default=2*(2**28)/2048,
        help='Set the number of vectors to accumulate between dumps. default is 2*(2^28)/2048, or just under 2 seconds.')
    p.add_option('-g', '--gain', dest='gain', type='int',default=0xffffffff,
        help='Set the digital gain (6bit quantisation scalar). Default is 0xffffffff (max), good for wideband noise. Set lower for CW tones.')
    p.add_option('-s', '--skip', dest='skip', action='store_true',
        help='Skip reprogramming the FPGA and configuring EQ.')
    opts, args = p.parse_args(sys.argv[1:])

    if args==[]:
        print 'Please specify a ROACH board. Run with the -h flag to see all options.\nExiting.'
        exit()
    else:
        roach = args[0]

try:

    #try:
            #arduino=serial.Serial('/dev/ttyUSB0',9600)
            #arduino.timeout=0.1
            #arduino.rtscts = False
            #arduino.xonxoff = False
    #except:
    #        print "Failed to connect to the Arduinos Readout"

    loggers = []
    lh=corr.log_handlers.DebugLogHandler()
    logger = logging.getLogger(roach)
    logger.addHandler(lh)
    logger.setLevel(10)

    print('Connecting to server %s on port %i... '%(roach,katcp_port)),
    fpga = corr.katcp_wrapper.FpgaClient(roach, katcp_port, timeout=10,logger=logger)
    time.sleep(1)


    

    #corr.iadc.set_mode(fpga,mode='SPI')
    #corr.iadc.analogue_gain_adj(fpga,0,gain_I=1.5,gain_Q=-1.5)
    #corr.iadc.analogue_gain_adj(fpga,1,gain_I=-1.5,gain_Q=-1.5)

    print '------------------------'
    print 'Programming FPGA...',
    if not opts.skip:
        fpga.progdev(bitstream)
        print 'done'
    else:
        print 'Skipped.'
	
    fpga.write_int('sync_en',1)
    #fpga.write_int('sync_rst',2**27-1)
	
    
    if fpga.is_connected():
        print 'ok\n'
    else:
        print 'ERROR connecting to server %s on port %i.\n'%(roach,katcp_port)
        exit_fail()
    
    fpga.write_int('phaseDelay1',32)# insert the time (in system clocks ie 4ns) between the phase switch command and the demodulation phase change
    fpga.write_int('demodPhaseSwitch',0) #Turn the demodulation on! Note that this is negative logic
    fpga.write_int('Demod_transL',5)# and give a window for zeroing the ADC samples uring the transition. Also in system clocks

    while(1):
	adcPos = getSnapCat('snapPos')
	adcNeg= getSnapCat('snapNeg')
	switch = getSnap('snap6')
	adcPosVals=numpy.array(adcPos)
	adcNegVals=numpy.array(adcNeg)
	switchVals=numpy.array(switch)
	print switch
	print numpy.size(adcPosVals)
	pylab.ion()
	pylab.figure(0)
	pylab.clf()
	pylab.xlabel('Time [ns]')
	pylab.ylabel('ADC Counts')
	x=numpy.arange(0,500)
	print numpy.size(x),numpy.size(adcPosVals)
	numpy.savetxt('ADCSamplesPos.txt',adcPosVals)
	numpy.savetxt('ADCSamplesNeg.txt',adcNegVals)
	pylab.plot(x,adcPosVals[x])
	pylab.draw()
	print '----------------------------------------------------------'
	fpga.write_int('gpio_set5',1)
    	fpga.write_int('gpio_set5',1)
	time.sleep(1)
    	fpga.write_int('gpio_set5',0)
    	fpga.write_int('gpio_set5',0)
	time.sleep(1)
	clock=fpga.read_uint('sync_cnt')
	print clock
	print 'Programming FPGA...'

except KeyboardInterrupt:
    exit_clean()
except:
    exit_fail()

exit_clean()

