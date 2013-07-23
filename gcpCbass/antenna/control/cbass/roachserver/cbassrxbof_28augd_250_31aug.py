#!/usr/bin/env python
'''
This script demonstrates programming an FPGA, configuring a wideband spectrometer and plotting the received data using the Python KATCP library along with the katcp_wrapper distributed in the corr package. Designed for use with TUT3 at the 2009 CASPER workshop.\n

You need to have KATCP and CORR installed. Get them from http://pypi.python.org/pypi/katcp and http://casper.berkeley.edu/svn/trunk/projects/packetized_correlator/corr-0.4.0/

\nAuthor: Jason Manley, November 2009.
'''

#TODO: add support for ADC histogram plotting.
#TODO: add support for determining ADC input level 

import corr,time,numpy,struct,sys,logging,pylab,serial

bitstreamprelim = 'cbassrx_28augd_250_2011_Aug_28_1420'
bitstream = bitstreamprelim+'.bof'
katcp_port=7147
sleep_time=0.005
fft_size=128 #size of fft in casper design i.e number of real channels = fftsize/2
LLf0msb='Subsystem1_ch1_msb'
LLf0lsb='Subsystem1_ch1_lsb'
LLf1msb='Subsystem1_ch5_msb'
LLf1lsb='Subsystem1_ch5_lsb'

RRf0msb='Subsystem1_ch4_msb'
RRf0lsb='Subsystem1_ch4_lsb'
RRf1msb='Subsystem1_ch8_msb'
RRf1lsb='Subsystem1_ch8_lsb'

Qf0msb='Subsystem1_ch2_msb'
Qf0lsb='Subsystem1_ch2_lsb'
Qf1msb='Subsystem1_ch6_msb'
Qf1lsb='Subsystem1_ch6_lsb'

Uf0msb='Subsystem1_ch3_msb'
Uf0lsb='Subsystem1_ch3_lsb'
Uf1msb='Subsystem1_ch7_msb'
Uf1lsb='Subsystem1_ch7_lsb'

Tref1f0msb='Subsystem1_ch9_msb'
Tref1f0lsb='Subsystem1_ch9_lsb'
Tref1f1msb='Subsystem1_ch10_msb'
Tref1f1lsb='Subsystem1_ch10_lsb'

Tref2f0msb='Subsystem1_ch11_msb'
Tref2f0lsb='Subsystem1_ch11_lsb'
Tref2f1msb='Subsystem1_ch12_msb'
Tref2f1lsb='Subsystem1_ch12_lsb'

fft0Evenmsb='Subsystem_ch2_msb'
fft0Evenlsb='Subsystem_ch2_lsb'
fft0Oddmsb='Subsystem_ch2_msb'
fft0Oddlsb='Subsystem_ch2_lsb'

fft1Evenlsb='Subsystem_ch3_lsb'
fft1Evenmsb='Subsystem_ch3_msb'
fft1Oddmsb='Subsystem_ch4_msb'
fft1Oddlsb='Subsystem_ch4_lsb'


fft2Evenmsb='Subsystem_ch5_msb'
fft2Evenlsb='Subsystem_ch5_lsb'
fft2Oddmsb='Subsystem_ch6_msb'
fft2Oddlsb='Subsystem_ch6_lsb'

fft3Evenlsb='Subsystem_ch7_lsb'
fft3Evenmsb='Subsystem_ch7_msb'
fft3Oddmsb='Subsystem_ch8_msb'
fft3Oddlsb='Subsystem_ch8_lsb'

fftreal=fft_size/2
snap3=[]
snap1=[]
fftstringUnsigned='>'+str(fftreal)+'I'
#fftstringUnsigned='>'+str(fftreal)+'l'
fftstringSigned='>'+str(fftreal)+'l'
ADCstringSigned='>'+str(fftreal)+'L'
phaseSwitch=3
delay='Double Feed Through ~5.0cm+CopperSection'
phaseDelay=0
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

def plot_spectrum(phaseSwitch,diode):
    
    global phaseDelay
	#get the data...  
    string=str(phaseDelay)+"\r\n"

    #/arduino.write(string)
    #if(phaseDelay>=64):
#	phaseDelay=0
    
    aa=numpy.zeros(32,long)
    ab=numpy.zeros(32,long)
    a=numpy.zeros(32,long)
    c=numpy.zeros(32,long)
    b=numpy.ones(32,long)
    e=numpy.ones(32,long)
    qap0=numpy.zeros(64,long)
    realinputvec=numpy.zeros(64,long)
    imaginputvec=numpy.zeros(64,long)
    ap0=numpy.zeros(64,long)
    ap1=numpy.zeros(64,long)
    ap2=numpy.zeros(64,long)
    ap3=numpy.zeros(64,long)
    ap4=numpy.zeros(64,long)
    ap5=numpy.zeros(64,long)
    ap6=numpy.zeros(64,long)
    ap7=numpy.zeros(64,long)
    ap0i=numpy.zeros(64,long)
    ap1i=numpy.zeros(64,long)
    ap2i=numpy.zeros(64,long)
    ap3i=numpy.zeros(64,long)
    ap4i=numpy.zeros(64,long)
    ap5i=numpy.zeros(64,long)
    ap6i=numpy.zeros(64,long)
    ap7i=numpy.zeros(64,long)
    ab=numpy.zeros(64,long)
    ac=numpy.zeros(64,long)
    i=10
    j=15
    i=0
    while(i<32):
	a[i]=1
	#rint a[i]
	i=i+1
    i=0
    a=a*10
    b=b*10
    a[19]=9	
    
    inputreal = open('inputreal.txt','r')
    realinput=inputreal.readlines()
    inputreal.close()
    
    inputimag = open('inputimag.txt','r')
    imaginput=inputimag.readlines()
    inputimag.close()
    
    ampreal0 = open('ampreal0.txt','r')
    a0=ampreal0.readlines()
    ampreal0.close()
    
    ampreal1 = open('ampreal1.txt','r')
    a1=ampreal1.readlines()
    ampreal1.close()
    
    ampreal2 = open('ampreal2.txt','r')
    a2=ampreal2.readlines()
    ampreal2.close()

    ampreal3 = open('ampreal3.txt','r')
    a3=ampreal3.readlines()
    ampreal3.close()
    
    ampreal4 = open('ampreal4.txt','r')
    a4=ampreal4.readlines()
    ampreal4.close()

    ampreal5 = open('ampreal5.txt','r')
    a5=ampreal5.readlines()
    ampreal5.close()


    ampreal6 = open('ampreal6.txt','r')
    a6=ampreal6.readlines()
    ampreal6.close()

    ampreal7 = open('ampreal7.txt','r')
    a7=ampreal7.readlines()
    ampreal7.close()
    
    ampimag0 = open('ampimag0.txt','r')
    ai0=ampimag0.readlines()
    ampimag0.close()
   # print 'testing',a1
    for i in range(0,32):
        imaginputvec[i]=int(imaginput[i])
        realinputvec[i]=int(realinput[i])
	ap0[i]=int(a0[i])
	ap1[i]=int(a1[i])
	ap2[i]=int(a2[i])
	ap3[i]=int(a3[i])
	ap4[i]=int(a4[i])
	ap5[i]=int(a5[i])
	ap6[i]=int(a6[i])
	ap7[i]=int(a7[i])
	ap0i[i]=int(ai0[i])

    bramw(fpga, 'amp_EQ0_coeff_real',ap0,64)
    bramw(fpga, 'amp_EQ1_coeff_real',ap1,64)
    bramw(fpga, 'amp_EQ2_coeff_real',ap2,64)
    bramw(fpga, 'amp_EQ3_coeff_real',ap3,64)
    bramw(fpga, 'amp_EQ4_coeff_imag',ap4,64)
    bramw(fpga, 'amp_EQ5_coeff_imag',ap5,64)
    bramw(fpga, 'amp_EQ6_coeff_imag',ap6,64)
    bramw(fpga, 'amp_EQ7_coeff_imag',ap7,64)
    
    LLeven_lsb=struct.unpack(fftstringUnsigned,fpga.read(LLf0lsb,fftreal*4,0))
    LLeven_msb=struct.unpack(fftstringUnsigned,fpga.read(LLf0msb,fftreal*4,0))
    LLodd_lsb=struct.unpack(fftstringUnsigned,fpga.read(LLf1lsb,fftreal*4,0))
    LLodd_msb=struct.unpack(fftstringUnsigned,fpga.read(LLf1msb,fftreal*4,0))

    Tref1even_lsb=struct.unpack(fftstringUnsigned,fpga.read(Tref1f0lsb,fftreal*4,0))
    Tref1even_msb=struct.unpack(fftstringUnsigned,fpga.read(Tref1f0msb,fftreal*4,0))
    Tref1odd_lsb=struct.unpack(fftstringUnsigned,fpga.read(Tref1f1lsb,fftreal*4,0))
    Tref1odd_msb=struct.unpack(fftstringUnsigned,fpga.read(Tref1f1msb,fftreal*4,0))
    
    RReven_lsb=struct.unpack(fftstringUnsigned,fpga.read(RRf0lsb,fftreal*4,0))
    RReven_msb=struct.unpack(fftstringUnsigned,fpga.read(RRf0msb,fftreal*4,0))
    RRodd_lsb=struct.unpack(fftstringUnsigned,fpga.read(RRf1lsb,fftreal*4,0))
    RRodd_msb=struct.unpack(fftstringUnsigned,fpga.read(RRf1msb,fftreal*4,0))

    Tref2even_lsb=struct.unpack(fftstringUnsigned,fpga.read(Tref2f0lsb,fftreal*4,0))
    Tref2even_msb=struct.unpack(fftstringUnsigned,fpga.read(Tref2f0msb,fftreal*4,0))
    Tref2odd_lsb=struct.unpack(fftstringUnsigned,fpga.read(Tref2f1lsb,fftreal*4,0))
    Tref2odd_msb=struct.unpack(fftstringUnsigned,fpga.read(Tref2f1msb,fftreal*4,0))
    
    Qeven_lsb=struct.unpack(fftstringSigned,fpga.read(Qf0lsb,fftreal*4,0))
    Qeven_msb=struct.unpack(fftstringSigned,fpga.read(Qf0msb,fftreal*4,0))
    Qodd_lsb=struct.unpack(fftstringSigned,fpga.read(Qf1lsb,fftreal*4,0))
    Qodd_msb=struct.unpack(fftstringSigned,fpga.read(Qf1msb,fftreal*4,0))
    
    Ueven_lsb=struct.unpack(fftstringSigned,fpga.read(Uf0lsb,fftreal*4,0))
    Ueven_msb=struct.unpack(fftstringSigned,fpga.read(Uf0msb,fftreal*4,0))
    Uodd_lsb=struct.unpack(fftstringSigned,fpga.read(Uf1lsb,fftreal*4,0))
    Uodd_msb=struct.unpack(fftstringSigned,fpga.read(Uf1msb,fftreal*4,0))


    fft0even_lsb=struct.unpack(fftstringUnsigned,fpga.read(fft0Evenlsb,fftreal*4,0))
    fft0even_msb=struct.unpack(fftstringUnsigned,fpga.read(fft0Evenmsb,fftreal*4,0))
    fft0odd_lsb=struct.unpack(fftstringUnsigned,fpga.read(fft0Oddlsb,fftreal*4,0))
    fft0odd_msb=struct.unpack(fftstringUnsigned,fpga.read(fft0Oddmsb,fftreal*4,0))

    fft1even_lsb=struct.unpack(fftstringUnsigned,fpga.read(fft1Evenlsb,fftreal*4,0))
    fft1even_msb=struct.unpack(fftstringUnsigned,fpga.read(fft1Evenmsb,fftreal*4,0))
    fft1odd_lsb=struct.unpack(fftstringUnsigned,fpga.read(fft1Oddlsb,fftreal*4,0))
    fft1odd_msb=struct.unpack(fftstringUnsigned,fpga.read(fft1Oddmsb,fftreal*4,0))

    fft2even_lsb=struct.unpack(fftstringUnsigned,fpga.read(fft2Evenlsb,fftreal*4,0))
    fft2even_msb=struct.unpack(fftstringUnsigned,fpga.read(fft2Evenmsb,fftreal*4,0))
    fft2odd_lsb=struct.unpack(fftstringUnsigned,fpga.read(fft2Oddlsb,fftreal*4,0))
    fft2odd_msb=struct.unpack(fftstringUnsigned,fpga.read(fft2Oddmsb,fftreal*4,0))

    fft3even_lsb=struct.unpack(fftstringUnsigned,fpga.read(fft3Evenlsb,fftreal*4,0))
    fft3even_msb=struct.unpack(fftstringUnsigned,fpga.read(fft3Evenmsb,fftreal*4,0))
    fft3odd_lsb=struct.unpack(fftstringUnsigned,fpga.read(fft3Oddlsb,fftreal*4,0))
    fft3odd_msb=struct.unpack(fftstringUnsigned,fpga.read(fft3Oddmsb,fftreal*4,0))
#   LLE=struct.unpack(fftstringSigned,fpga.read(LLf0,fftreal*4,0))
 #  LLO=struct.unpack(fftstringSigned,fpga.read(LLf1,fftreal*4,0))

   

    for i in range(0,32):
	aa[i]=int(fft3even_lsb[i])
	ab[i]=int(fft2even_lsb[i])


    #print fftbeforeampreal
    #print fftafterampreal

    pylab.figure(num=1,figsize=(15,15),dpi=80)
	
    #print fft0odd_lsb

    f=pylab.gcf()
    pylab.clf()
   
    
   # pylab.subplot(211)
    #pylab.plot(fft0even_lsb,'o',label='|fft0|^2')
    #pylab.ylim(0.0e8,3e8)
    #pylab.xlim(-1,fftreal/2-1)
    #pylab.title('testing')
    #pylab.subplot(812)
    #pylab.plot(fft0even_msb,'o',label='|fft0|^2')
    #pylab.grid()
    #pylab.ylim(0.0e8,3e8)
    #pylab.xlim(-1,fftreal/2-1)
    #pylab.subplot(814)
    #pylab.plot(fft0odd_lsb,'o',label='|fft0|^2')
    #pylab.grid()
    #pylab.xlim(-1,fftreal/2-1)
 #   pylab.plot(fft1Even,'o',label='|fft0|^2')
    diodestate='Noise Diode State '+str(diode)
    pylab.title(diodestate)
    pylab.subplot(441)
    pylab.title('RRf0even_msb')
    pylab.plot(RReven_msb,'o',label='RRf0even_msb')
    pylab.grid()
    pylab.xlim(-1,fftreal/2-1)
    pylab.subplot(442)
    pylab.title('RRf0even_lsb')
    pylab.plot(RReven_lsb,'o',label='|fft2|^2')
    pylab.grid()
    pylab.xlim(-1,fftreal/2-1)
    f.suptitle(diodestate, fontsize=16)
    

    pylab.subplot(443)
    pylab.title('Tref2even_msb')
    pylab.plot(Tref2even_msb,'o',label='Tref2even_msb')
    pylab.grid()
    pylab.xlim(-1,fftreal/2-1)
    pylab.subplot(444)
    pylab.title('Tref2even_lsb')
    pylab.plot(Tref2even_lsb,'o',label='|fft2|^2')
    pylab.grid()
    pylab.xlim(-1,fftreal/2-1)
    f.suptitle(diodestate, fontsize=16)
    
    pylab.subplot(445)
    pylab.title('Qeven_msb')
    pylab.plot(Qeven_msb,'o',label='Qeven_msb')
    pylab.grid()
    pylab.xlim(-1,fftreal/2-1)
    pylab.subplot(446)
    pylab.title('Qeven_lsb')
    pylab.plot(Qeven_lsb,'o',label='|fft2|^2')
    pylab.grid()
    pylab.xlim(-1,fftreal/2-1)
    f.suptitle(diodestate, fontsize=16)
    

    pylab.subplot(447)
    pylab.title('Ueven_msb')
    pylab.plot(Ueven_msb,'o',label='Ueven_msb')
    pylab.grid()
    pylab.xlim(-1,fftreal/2-1)
    pylab.subplot(448)
    pylab.title('Ueven_lsb')
    pylab.plot(Ueven_lsb,'o',label='|fft2|^2')
    pylab.grid()
    pylab.xlim(-1,fftreal/2-1)
    f.suptitle(diodestate, fontsize=16)



    pylab.subplot(449)
    f.suptitle(diodestate, fontsize=16)
    pylab.title('fft2^2 Even MSB')
    pylab.plot(fft2even_msb,'o',label='|fft2|^2')
    pylab.xlim(-1,fftreal/2-1)
    pylab.grid()
    pylab.subplot(4,4,10)
    pylab.title('fft2^2 Even LSB')
    pylab.plot(fft2even_lsb,'o',label='|fft2|^2')
  #  pylab.ylim(250000,290000)
    pylab.grid()
    pylab.xlim(-1,fftreal/2-1)

    pylab.subplot(4,4,11)
    f.suptitle(diodestate, fontsize=16)
    pylab.title('fft3^2 Even MSB')
    pylab.plot(fft3even_msb,'o',label='|fft2|^2')
    pylab.xlim(-1,fftreal/2-1)
    pylab.grid()
    pylab.subplot(4,4,12)
    pylab.title('fft3^2 Even LSB')
    pylab.plot(aa,'o',label='|fft2|^2')
    pylab.grid()
    pylab.xlim(-1,fftreal/2-1)
   # pylab.ylim(250000,290000)
  #  pylab.xlim(-1,fftreal/2)
    #pylab.subplot(814)
    #pylab.plot(snaparray3,'.',label='ADC1')
    #pylab.plot(snaparray1,'x',label='ADC2')
    #pylab.legend()
    print 'I', RReven_msb
    print '-------------'
    print 'Tref', Tref2even_msb
    print '-------------'
    print fft2even_msb
    print '-------------'
    print fft3even_msb 
    
    pylab.draw()
    pylab.show()
    #size=f.get_size_inches()
    

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

    if fpga.is_connected():
        print 'ok\n'
    else:
        print 'ERROR connecting to server %s on port %i.\n'%(roach,katcp_port)
        exit_fail()

    

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

    print 'Configuring accumulation period...',
    fpga.write_int('acc_len',opts.acc_len)
    print 'done',str(opts.acc_len)

    print 'Resetting counters...',
    fpga.write_int('cnt_rst',1) 
    fpga.write_int('cnt_rst',0) 
    fpga.write_int('snap_we',0)
    fpga.write_int('snap_we',1)
    fpga.write_int('sync_en',1)
    fpga.write_int('snap_we',1)
    fpga.write_int('ctrl_sw',43690) #fft size
    #fpga.write_int('ctrl_sw',2**5-1) #fft size



	


    print 'done'

    time.sleep(2)

    prev_integration = fpga.read_uint('acc_cnt')
    number=0
    diode=0
    while(number<=16):
        #fpga.write_int('phaseSwitch',number)
        #fpga.write_int('acc_len',15000000)
        fpga.write_int('acc_len',opts.acc_len)
        prev_integration = fpga.read_uint('acc_cnt')
        current_integration = fpga.read_uint('acc_cnt')
        diff=current_integration - prev_integration
#        print 'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx'
	while diff==0:
            current_integration = fpga.read_uint('acc_cnt')
            diff=current_integration - prev_integration
            time.sleep(0.005)
	    #print current_integration, prev_integration
	 #   print 'sleeping'	
        
        if diff > 1:
 #           print 'WARN: We lost %i integrations!'%(current_integration - prev_integration)
	    prev_integration = fpga.read_uint('acc_cnt')
	    #fpga.write_int('snap_we',1)
	    #fpga.write_int('snap_we',0)
	    #fpga.write_int('snap_we',1)


    	#fpga.write_int('snap_we',0)
	
	  

	plot_spectrum(number,diode)
#	plot_spectrumflat(number,1)
    	number=number+1
	if(number==16):
		number=0

	time.sleep(1)

except KeyboardInterrupt:
    exit_clean()
except:
    exit_fail()

exit_clean()

