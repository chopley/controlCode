#!/usr/bin/env python
'''
This script allows testing of the phase switch modulation and demodulation on C-BASS- inject a CW source (preferably quite low as it's easier to see the phase!) around 4520MHz into the C-BASS input port. Run this script and you will get two output files ADCSamplesNeg.txt and ADCSamplesPos.txt. These will show you the actual ADC values that are being presented to the FFT after the demodulation . You can adjust the actual position of the demodulation and the blanking window- just look at the appropriate registers. Note that the design begins with demodulation- so to turn it off you have to write a 1 to a register.
'''

#TODO: add support for ADC histogram plotting.
#TODO: add support for determining ADC input level 

import corr,time,numpy,struct,sys,logging,pylab,serial

bitstreamprelim = 'cbassrx_2may_pow_2012_May_02_1555'
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

def readIntegration64bit(nameeven="",nameodd=""):
     	
    evenBram=nameeven+'_msb'
    oddBram=nameodd+'_msb'
    evenBramlsb=nameeven+'_lsb'
    oddBramlsb=nameodd+'_lsb'
    evenMSB=fpga.read(evenBram,32*4,0)  	    
    oddMSB=fpga.read(oddBram,32*4,0)
    evenLSB=fpga.read(evenBramlsb,32*4,0)  	    
    oddLSB=fpga.read(oddBramlsb,32*4,0)
    fullSpectrum=numpy.zeros(64)
    
    evenUnpackedMSB=struct.unpack('>32I',evenMSB)
    oddUnpackedMSB=struct.unpack('>32I',oddMSB)
    
    evenUnpackedLSB=struct.unpack('>32I',evenLSB)
    oddUnpackedLSB=struct.unpack('>32I',oddLSB)
    fullSpectrum=numpy.zeros(64)
   
	
    evenarrayLSB=numpy.array(evenUnpackedLSB,dtype=numpy.uint32)
    evenarrayMSB=numpy.array(evenUnpackedMSB,dtype=numpy.uint32)
    oddarrayLSB=numpy.array(oddUnpackedLSB,dtype=numpy.uint32)
    oddarrayMSB=numpy.array(oddUnpackedMSB,dtype=numpy.uint32)
    #print 'MSB',evenarrayMSB,evenarrayLSB 
    aa=numpy.array(evenarrayLSB,dtype=numpy.uint64)
    ab=numpy.array(evenarrayMSB,dtype=numpy.uint64)
    ab=ab<<32
    ac=aa|ab
    even=numpy.array(ac,dtype=numpy.uint64)
    
    aa=numpy.array(oddarrayLSB,dtype=numpy.uint64)
    ab=numpy.array(oddarrayMSB,dtype=numpy.uint64)
    ab=ab<<32
    ac=aa|ab
    odd=numpy.array(ac,dtype=numpy.uint64)

    #print 'even',numpy.mean(numpy.array(even)) 
    #print 'odd', numpy.mean(numpy.array(odd)) 
    fullSpectrum[::2]=numpy.array(even)	
    fullSpectrum[1::2]=numpy.array(odd)
    return fullSpectrum 	

def bramr(fpga, bname, samples=1024):
    bramvals=fpga.read(bname,samples*4,0)  	    
    string = '>'+str(samples)+'i'
    b_0=struct.unpack(string,bramvals)
    data=numpy.array(b_0)	
    return data

def initialiseCoefficients():
	a=100*numpy.ones(32)	
	b=1*numpy.zeros(32)	
	bramw(fpga,'amp_EQ0_coeff_real', a, 32)
	bramw(fpga,'amp_EQ1_coeff_real', a, 32)
	bramw(fpga,'amp_EQ2_coeff_real', a, 32)
	bramw(fpga,'amp_EQ3_coeff_real', a, 32)
	bramw(fpga,'amp_EQ4_coeff_real', a, 32)
	bramw(fpga,'amp_EQ5_coeff_real', a, 32)
	bramw(fpga,'amp_EQ6_coeff_real', a, 32)
	bramw(fpga,'amp_EQ7_coeff_real', a, 32)
	bramw(fpga,'amp_EQ0_coeff_imag', b, 32)
	bramw(fpga,'amp_EQ1_coeff_imag', b, 32)
	bramw(fpga,'amp_EQ2_coeff_imag', b, 32)
	bramw(fpga,'amp_EQ3_coeff_imag', b, 32)
	bramw(fpga,'amp_EQ4_coeff_imag', b, 32)
	bramw(fpga,'amp_EQ5_coeff_imag', b, 32)
	bramw(fpga,'amp_EQ6_coeff_imag', b, 32)
	bramw(fpga,'amp_EQ7_coeff_imag', b, 32)
	b0=bramr(fpga,'amp_EQ0_coeff_real',32)
	b1=bramr(fpga,'amp_EQ1_coeff_real',32)
	b2=bramr(fpga,'amp_EQ2_coeff_real',32)
	b3=bramr(fpga,'amp_EQ3_coeff_real',32)
	b4=bramr(fpga,'amp_EQ4_coeff_real',32)
	b5=bramr(fpga,'amp_EQ5_coeff_real',32)
	b6=bramr(fpga,'amp_EQ6_coeff_real',32)
	b7=bramr(fpga,'amp_EQ7_coeff_real',32)
	print b0,b1#,b2,b3,b4,b5,b6,b7
	b0=bramr(fpga,'amp_EQ0_coeff_imag',32)
	b1=bramr(fpga,'amp_EQ1_coeff_imag',32)
	b2=bramr(fpga,'amp_EQ2_coeff_imag',32)
	b3=bramr(fpga,'amp_EQ3_coeff_imag',32)
	b4=bramr(fpga,'amp_EQ4_coeff_imag',32)
	b5=bramr(fpga,'amp_EQ5_coeff_imag',32)
	b6=bramr(fpga,'amp_EQ6_coeff_imag',32)
	b7=bramr(fpga,'amp_EQ7_coeff_imag',32)
	print b0,b1#,b3,b4,b5,b6,b7

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

	fpga.write_int('cnt_rst',1) 
	fpga.write_int('cnt_rst',0) 
	fpga.write_int('snap_we',0)
	fpga.write_int('snap_we',1)
	fpga.write_int('sync_en',1)
	fpga.write_int('snap_we',1)
	fpga.write_int('ctrl_sw',43690) #fft size
	fpga.write_int('acc_sync_delay',10) #fft size
	fpga.write_int('phaseDelay2',30)# insert the time (in system clocks ie 4ns) between the phase switch command and the demodulation phase change
	fpga.write_int('demodPhaseSwitch1',0) #Turn the demodulation on! Note that this is negative logic
	fpga.write_int('Demod1_transL',7)# and give a window for zeroing the ADC samples uring the transition. Also in system clocks
	fpga.write_int('Demod2_transL',7)# and give a window for zeroing the ADC samples uring the transition. Also in system clocks
	fpga.write_int('Demod3_transL',7)# and give a window for zeroing the ADC samples uring the transition. Also in system clocks
	fpga.write_int('Demod4_transL',7)# and give a window for zeroing the ADC samples uring the transition. Also in system clocks
	fpga.write_int('phaseFrequency',1280)# frequency = 24.26kHz
	fpga.write_int('enablePhaseSwitch',1)# frequency = 24.26kHz
	initialiseCoefficients()

	fpga.write_int('ADCedgePolarity',0)# 
	adcPos1 = getSnapCat('ADCChannel1')
	adcPos2 = getSnapCat('ADCChannel2')
	adcPos3 = getSnapCat('ADCChannel3')
	adcPos4 = getSnapCat('ADCChannel4')
	fpga.write_int('ADCedgePolarity',1)# 
	time.sleep(0.1)
	adcNeg1= getSnapCat('ADCChannel1')
	adcNeg2= getSnapCat('ADCChannel2')
	adcNeg3= getSnapCat('ADCChannel3')
	adcNeg4= getSnapCat('ADCChannel4')
	adcPosVals1=numpy.array(adcPos1)
	adcNegVals1=numpy.array(adcNeg1)
	adcPosVals2=numpy.array(adcPos2)
	adcNegVals2=numpy.array(adcNeg2)
	adcPosVals3=numpy.array(adcPos3)
	adcNegVals3=numpy.array(adcNeg3)
	adcPosVals4=numpy.array(adcPos4)
	adcNegVals4=numpy.array(adcNeg4)
	#switchVals=numpy.array(switch)
	print numpy.size(adcPosVals1)
	pylab.ion()
	pylab.figure(0)
	pylab.clf()
	pylab.xlabel('Time [ns]')
	pylab.ylabel('ADC Counts')
	x=numpy.arange(50,200)
	print numpy.size(x),numpy.size(adcPosVals1)
	numpy.savetxt('ADCSamplesPos1.txt',adcPosVals1)
	numpy.savetxt('ADCSamplesNeg1.txt',adcNegVals1)
	numpy.savetxt('ADCSamplesPos2.txt',adcPosVals2)
	numpy.savetxt('ADCSamplesNeg2.txt',adcNegVals2)
	numpy.savetxt('ADCSamplesPos3.txt',adcPosVals3)
	numpy.savetxt('ADCSamplesNeg3.txt',adcNegVals3)
	numpy.savetxt('ADCSamplesPos4.txt',adcPosVals4)
	numpy.savetxt('ADCSamplesNeg4.txt',adcNegVals4)
	pylab.plot(x,adcPosVals1[x])
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
	prev_integration = fpga.read_uint('Subsystem1_readAccumulation')
	integration = fpga.read_uint('Subsystem1_readAccumulation')

	while(1):
		diff=integration - prev_integration
		while diff==0:
				integration = fpga.read_uint('Subsystem1_readAccumulation')
				diff=integration - prev_integration
				print integration
		prev_integration=integration
		Channel1On=readIntegration64bit('Subsystem1_ch1','Subsystem1_ch2')
		Channel1On=10*numpy.log10(numpy.mean(Channel1On))
		c="%s,%s\n" % (Channel1On,str(integration))
		f = open("data.txt", 'a')
		print c
		#time.sleep(0.1)	
		f.write(c)
		f.close()

except KeyboardInterrupt:
    exit_clean()
except:
    exit_fail()

exit_clean()

