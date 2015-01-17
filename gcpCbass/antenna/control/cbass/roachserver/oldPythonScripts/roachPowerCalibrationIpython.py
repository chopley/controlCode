#!/usr/bin/env python
'''
C-BASS power calibration code
You need to have KATCP and CORR installed. Get them from http://pypi.python.org/pypi/katcp and http://casper.berkeley.edu/svn/trunk/projects/packetized_correlator/corr-0.4.0/

\nAuthor: Charles Copley, January 20122.
'''

#TODO: add support for ADC histogram plotting.
#TODO: add support for determining ADC input level 

import corr,time,numpy,struct,sys,logging,pylab,serial

bitstreampower = 'cbassrx_2mar_pow_2012_Mar_02_1638'
bitstreampower = bitstreampower+'.bof'
katcp_port=7147
sleep_time=0.005
desiredPower=1e15


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

def readIntegration64bit(fpga,nameeven="",nameodd=""):
     	
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

def readIntegration(nameeven="",nameodd=""):
     	
    evenBram=nameeven+'_lsb'
    oddBram=nameodd+'_lsb'
    even=fpga.read(evenBram,32*4,0)  	    
    odd=fpga.read(oddBram,32*4,0)
    evenUnpacked=struct.unpack('>32I',even)
    oddUnpacked=struct.unpack('>32I',odd)
    fullSpectrum=numpy.zeros(64)
    
    fullSpectrum[::2]=numpy.array(evenUnpacked)	
    fullSpectrum[1::2]=numpy.array(oddUnpacked)
    return fullSpectrum 	


def plotSpectrum(spectrum,filename=""):
		pylab.ion() 	
		pylab.figure(0) 	
		pylab.clf() 	
		pylab.plot(spectrum)
		pylab.draw()
		pylab.savefig(filename)

def plotSpectrum4(spectrum1On,spectrum2On,spectrum3On,spectrum4On,spectrum1Off,spectrum2Off,spectrum3Off,spectrum4Off,filename=""):
		pylab.ion() 	
		pylab.figure(0) 	
		pylab.clf() 
		pylab.grid()
		pylab.subplot(221)
		pylab.plot(10*numpy.log10(spectrum1On),'r')
		pylab.plot(10*numpy.log10(spectrum1Off))
#		pylab.ylim([119,125])
		pylab.title('Channel1')
		pylab.subplot(222)
		pylab.title('Channel2')
		pylab.plot(10*numpy.log10(spectrum2On),'r')
		pylab.plot(10*numpy.log10(spectrum2Off))
#		pylab.ylim([119,125])
		pylab.subplot(223)
		pylab.title('Channel3')
		pylab.plot(10*numpy.log10(spectrum3On),'r')
		pylab.plot(10*numpy.log10(spectrum3Off))
#		pylab.ylim([119,125])
		pylab.subplot(224)
		pylab.title('Channel4')
		pylab.plot(10*numpy.log10(spectrum4On),'r')
		pylab.plot(10*numpy.log10(spectrum4Off))
#		pylab.ylim([119,125])
		pylab.draw()
		pylab.savefig(filename)
    
def plotSpectrum6(spectrum1On,spectrum2On,spectrum3On,spectrum4On,spectrum1Off,spectrum2Off,spectrum3Off,spectrum4Off,thetaL,thetaR,filename=""):
		pylab.ion() 	
		pylab.figure(0) 	
		pylab.clf() 
		pylab.grid()
		pylab.subplot(321)
		pylab.plot(spectrum1On,'r')
		pylab.plot(spectrum1Off)
		pylab.title('Channel1')
		pylab.subplot(322)
		pylab.title('Channel2')
		pylab.plot(spectrum2On,'r')
		pylab.plot(spectrum2Off)
		pylab.subplot(323)
		pylab.title('Channel3')
		pylab.plot(spectrum3On,'r')
		pylab.plot(spectrum3Off)
		pylab.subplot(324)
		pylab.title('Channel4')
		pylab.plot(spectrum4On,'r')
		pylab.plot(spectrum4Off)
		pylab.subplot(325)
		pylab.title('ThetaL')
		pylab.plot(thetaL,'r')
		pylab.subplot(326)
		pylab.title('ThetaR')
		pylab.plot(thetaR,'r')
		pylab.draw()
		pylab.savefig(filename)



    
def bramr(fpga, bname, samples=1024):
    bramvals=fpga.read(bname,samples*4,0)  	    
    string = '>'+str(samples)+'i'
    b_0=struct.unpack(string,bramvals)
    data=numpy.array(b_0)	
    return data

def updateIntegration(fpga):    
		#time.sleep(0.2)
		prev_integration = fpga.read_uint('Subsystem1_readAccumulation')
		current_integration = fpga.read_uint('Subsystem1_readAccumulation')
		#prev_integration = fpga.read_uint('acc_cnt')
		#current_integration = fpga.read_uint('acc_cnt')
		diff=current_integration - prev_integration

		while diff==0:
			#	current_integration = fpga.read_uint('acc_cnt')
				current_integration = fpga.read_uint('Subsystem1_readAccumulation')
				diff=current_integration - prev_integration
		#time.sleep(1)
#		print diff,current_integration
		
		if diff > 1:
				print 'WARN: We lost %i integrations!'%(current_integration - prev_integration)
#		print prev_integration,current_integration
		return current_integration
def diodeState(a):
    fpga.write_int('gpio_set4',a)
    #fpga.write_int('gpio1_set',1)
    return a

def diodeOff():
    fpga.write_int('gpio_set4',0)
    #fpga.write_int('gpio1_set',0)
    return 0

def diodeOn(fpga):
    fpga.write_int('gpio_set4',1)
    #fpga.write_int('gpio1_set',1)
    return 1

def diodeOff(fpga):
    fpga.write_int('gpio_set4',0)
    #fpga.write_int('gpio1_set',0)
    return 0

def diodeState(state):
    if(state==1):#turn diode on
    #	fpga.write_int('gpio_set',0)
    #	fpga.write_int('gpio1_set',1)
	print 'turning on'
    elif(state==0):#turn diode off
    #	fpga.write_int('gpio_set',1)
    #	fpga.write_int('gpio1_set',0)
	print 'turning on'
    return state	
   
def initialiseCoefficients(fpga):
	a=1000*numpy.ones(32)	
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

def gainCalibration(Channel1,Channel2,Channel3,Channel4):
	#first initialise the cofficients vectors
	Coffs1R=numpy.zeros(64)
	Coffs2R=numpy.zeros(64)
	Coffs3R=numpy.zeros(64)
	Coffs4R=numpy.zeros(64)
	Coffs1I=numpy.zeros(64)
	Coffs2I=numpy.zeros(64)
	Coffs3I=numpy.zeros(64)
	Coffs4I=numpy.zeros(64)

	b0r=bramr(fpga,'amp_EQ0_coeff_real',32)
	b1r=bramr(fpga,'amp_EQ1_coeff_real',32)
	b2r=bramr(fpga,'amp_EQ2_coeff_real',32)
	b3r=bramr(fpga,'amp_EQ3_coeff_real',32)
	b4r=bramr(fpga,'amp_EQ4_coeff_real',32)
	b5r=bramr(fpga,'amp_EQ5_coeff_real',32)
	b6r=bramr(fpga,'amp_EQ6_coeff_real',32)
	b7r=bramr(fpga,'amp_EQ7_coeff_real',32)

	b0i=bramr(fpga,'amp_EQ0_coeff_imag',32)
	b1i=bramr(fpga,'amp_EQ1_coeff_imag',32)
	b2i=bramr(fpga,'amp_EQ2_coeff_imag',32)
	b3i=bramr(fpga,'amp_EQ3_coeff_imag',32)
	b4i=bramr(fpga,'amp_EQ4_coeff_imag',32)
	b5i=bramr(fpga,'amp_EQ5_coeff_imag',32)
	b6i=bramr(fpga,'amp_EQ6_coeff_imag',32)
	b7i=bramr(fpga,'amp_EQ7_coeff_imag',32)
	
	desiredResponse=desiredPower#desired power levels
	nIntegrations=78125#number of integrations
	#now calculate the squared required coefficientns normalised to the desired respone
        #Channel1[2:63]= savitzky_golay(Channel1[2:63],7,3)
        #Channel2[2:63]= savitzky_golay(Channel2[2:63],7,3)
        #Channel3[2:63]= savitzky_golay(Channel3[2:63],7,3)
        #Channel4[2:63]= savitzky_golay(Channel4[2:63],7,3)
	Channel1Coffs=desiredResponse/Channel1
	Channel2Coffs=desiredResponse/Channel2
	Channel3Coffs=desiredResponse/Channel3
	Channel4Coffs=desiredResponse/Channel4
	#now square root them
	
	Coffs1R=numpy.sqrt(Channel1Coffs)
	Coffs2R=numpy.sqrt(Channel2Coffs)
	Coffs3R=numpy.sqrt(Channel3Coffs)
	Coffs4R=numpy.sqrt(Channel4Coffs)

	print 'Channel1Coffs\n',Channel1Coffs,b0r,b1r,b0i,b1r,Channel1
	#and multiply by the original coefficients
	Coffs1R[::2]=Coffs1R[::2]*b0r	
	Coffs1R[1::2]=Coffs1R[1::2]*b1r	
	Coffs2R[::2]=Coffs2R[::2]*b2r	
	Coffs2R[1::2]=Coffs2R[1::2]*b3r	
	Coffs3R[::2]=Coffs3R[::2]*b4r	
	Coffs3R[1::2]=Coffs3R[1::2]*b5r	
	Coffs4R[::2]=Coffs4R[::2]*b6r	
	Coffs4R[1::2]=Coffs4R[1::2]*b7r	
	
	print 'CoffsFirst\n',Coffs3R[30:34],Coffs3I[30:34],Channel3[30:34]

	
        #Coffs1R= savitzky_golay(Coffs1R,5,2)
        #Coffs2R= savitzky_golay(Coffs2R,5,2)
       # Coffs3R= savitzky_golay(Coffs3R,5,2)
       # Coffs4R= savitzky_golay(Coffs4R,5,2)
       # Coffs1I= savitzky_golay(Coffs1I,5,2)
       # Coffs2I= savitzky_golay(Coffs2I,5,2)
        #Coffs3I= savitzky_golay(Coffs3I,5,2)
        #Coffs4I= savitzky_golay(Coffs4I,5,2)
	
	[Coffs1R,Coffs1I,Coffs2R,Coffs2I,Coffs3R,Coffs3I,Coffs4R,Coffs4I]=writeCoffs(Coffs1R,Coffs1I,Coffs2R,Coffs2I,Coffs3R,Coffs3I,Coffs4R,Coffs4I,fpga)
	
	return Coffs1R,Coffs1I,Coffs2R,Coffs2I,Coffs3R,Coffs3I,Coffs4R,Coffs4I

### This is the Gaussian data smoothing function I wrote ###  

def smoothListGaussian(list,degree=5):  
     window=degree*2-1  
     weight=numpy.array([1.0]*window)  
     weightGauss=[]  

     for i in range(window):  
         i=i-degree+1  
         frac=i/float(window)  
         gauss=1/(numpy.exp((4*(frac))**2))  
         weightGauss.append(gauss)  

     weight=numpy.array(weightGauss)*weight  
     smoothed=[0.0]*(len(list)-window)  

     for i in range(len(smoothed)):  
         smoothed[i]=sum(numpy.array(list[i:i+window])*weight)/sum(weight)  
     return smoothed  


	
 
def Step2PhaseCorrection(LCPOn,LCPOff,RCPOn,RCPOff,LCPRefOn,LCPRefOff,RCPRefOn,RCPRefOff):
	Coffs1R=numpy.zeros(64)
	Coffs2R=numpy.zeros(64)
	Coffs3R=numpy.zeros(64)
	Coffs4R=numpy.zeros(64)
	Coffs1I=numpy.zeros(64)
	Coffs2I=numpy.zeros(64)
	Coffs3I=numpy.zeros(64)
	Coffs4I=numpy.zeros(64)
	
	Coffs2Complex=numpy.zeros(64,complex)
	Coffs4Complex=numpy.zeros(64,complex)

	b0r=bramr(fpga,'amp_EQ0_coeff_real',32)
	b1r=bramr(fpga,'amp_EQ1_coeff_real',32)
	b2r=bramr(fpga,'amp_EQ2_coeff_real',32)
	b3r=bramr(fpga,'amp_EQ3_coeff_real',32)
	b4r=bramr(fpga,'amp_EQ4_coeff_real',32)
	b5r=bramr(fpga,'amp_EQ5_coeff_real',32)
	b6r=bramr(fpga,'amp_EQ6_coeff_real',32)
	b7r=bramr(fpga,'amp_EQ7_coeff_real',32)

	b0i=bramr(fpga,'amp_EQ0_coeff_imag',32)
	b1i=bramr(fpga,'amp_EQ1_coeff_imag',32)
	b2i=bramr(fpga,'amp_EQ2_coeff_imag',32)
	b3i=bramr(fpga,'amp_EQ3_coeff_imag',32)
	b4i=bramr(fpga,'amp_EQ4_coeff_imag',32)
	b5i=bramr(fpga,'amp_EQ5_coeff_imag',32)
	b6i=bramr(fpga,'amp_EQ6_coeff_imag',32)
	b7i=bramr(fpga,'amp_EQ7_coeff_imag',32)

	Coffs1R[::2]=b0r
	Coffs1R[1::2]=b1r
	Coffs2R[::2]=b2r
	Coffs2R[1::2]=b3r
	Coffs3R[::2]=b4r
	Coffs3R[1::2]=b5r
	Coffs4R[::2]=b6r
	Coffs4R[1::2]=b7r
	
	Coffs1I[::2]=b0i
	Coffs1I[1::2]=b1i
	Coffs2I[::2]=b2i
	Coffs2I[1::2]=b3i
	Coffs3I[::2]=b4i
	Coffs3I[1::2]=b5i
	Coffs4I[::2]=b6i
	Coffs4I[1::2]=b7i
	
	Coffs2Complex = Coffs2R+1j*Coffs2I
	Coffs4Complex = Coffs4R+1j*Coffs4I
	

	dL=LCPOn-LCPOff
	dR=RCPOn-RCPOff

	dLRef=LCPRefOn-LCPRefOff
	dRRef=RCPRefOn-RCPRefOff
	#print dL,dR
	#print dLRef,dRRef
	t1=dL/dLRef-1.
	t2=dL/dLRef+1.
	t3=dR/dRRef-1.
	t4=dR/dRRef+1.
	thetaL=numpy.arccos(t1/t2)
	thetaR=numpy.arccos(t3/t4)
	thetaL=numpy.nan_to_num(thetaL)
	thetaR=numpy.nan_to_num(thetaR)
	
	Coffs2Complex[30]=Coffs2Complex[30]*numpy.exp(1j*0.7)
	Coffs4Complex[30]=Coffs4Complex[30]*numpy.exp(1j*0.7)
#	Coffs2Complex=Coffs2Complex*numpy.exp(1j*thetaR)
#	Coffs4Complex=Coffs4Complex*numpy.exp(1j*-thetaL)
	
	Coffs2R=numpy.real(Coffs2Complex)
	Coffs2I=numpy.imag(Coffs2Complex)
	Coffs4R=numpy.real(Coffs4Complex)
	Coffs4I=numpy.imag(Coffs4Complex)

	print 'real',numpy.real(Coffs2Complex),'imag',numpy.imag(Coffs2Complex)
	[Coffs1R,Coffs1I,Coffs2R,Coffs2I,Coffs3R,Coffs3I,Coffs4R,Coffs4I]=writeCoffs(Coffs1R,Coffs1I,Coffs2R,Coffs2I,Coffs3R,Coffs3I,Coffs4R,Coffs4I)
	return thetaL,thetaR

def savitzky_golay(y, window_size, order, deriv=0):
	r"""Smooth (and optionally differentiate) data with a Savitzky-Golay filter.
	The Savitzky-Golay filter removes high frequency noise from data.
	It has the advantage of preserving the original shape and
	features of the signal better than other types of filtering
	approaches, such as moving averages techhniques.
	Parameters
	----------
	y : array_like, shape (N,)
	 the values of the time history of the signal.
	window_size : int
	 the length of the window. Must be an odd integer number.
	order : int
	 the order of the polynomial used in the filtering.
	 Must be less then `window_size` - 1.
	deriv: int
	 the order of the derivative to compute (default = 0 means only smoothing)
	Returns
	-------
	ys : ndarray, shape (N)
	 the smoothed signal (or it's n-th derivative).
	Notes
	-----
	The Savitzky-Golay is a type of low-pass filter, particularly
	suited for smoothing noisy data. The main idea behind this
	approach is to make for each point a least-square fit with a
	polynomial of high order over a odd-sized window centered at
	the point.
	Examples
	--------
	t = np.linspace(-4, 4, 500)
	y = np.exp( -t**2 ) + np.random.normal(0, 0.05, t.shape)
	ysg = savitzky_golay(y, window_size=31, order=4)
	import matplotlib.pyplot as plt
	plt.plot(t, y, label='Noisy signal')
	plt.plot(t, np.exp(-t**2), 'k', lw=1.5, label='Original signal')
	plt.plot(t, ysg, 'r', label='Filtered signal')
	plt.legend()
	plt.show()
	References
	----------
	.. [1] A. Savitzky, M. J. E. Golay, Smoothing and Differentiation of
	Data by Simplified Least Squares Procedures. Analytical
	Chemistry, 1964, 36 (8), pp 1627-1639.
	.. [2] Numerical Recipes 3rd Edition: The Art of Scientific Computing
	W.H. Press, S.A. Teukolsky, W.T. Vetterling, B.P. Flannery
	Cambridge University Press ISBN-13: 9780521880688
	"""
	try:
	 window_size = numpy.abs(numpy.int(window_size))
	 order = numpy.abs(numpy.int(order))
	except ValueError, msg:
	 raise ValueError("window_size and order have to be of type int")
	if window_size % 2 != 1 or window_size < 1:
	 raise TypeError("window_size size must be a positive odd number")
	if window_size < order + 2:
	 raise TypeError("window_size is too small for the polynomials order")
	order_range = range(order+1)
	half_window = (window_size -1) // 2
	# precompute coefficients
	b = numpy.mat([[k**i for i in order_range] for k in range(-half_window, half_window+1)])
	m = numpy.linalg.pinv(b).A[deriv]
	# pad the signal at the extremes with
	# values taken from the signal itself
	firstvals = y[0] - numpy.abs( y[1:half_window+1][::-1] - y[0] )
	lastvals = y[-1] + numpy.abs(y[-half_window-1:-1][::-1] - y[-1])
	y = numpy.concatenate((firstvals, y, lastvals))
	return numpy.convolve( m, y, mode='valid')	
	
def getPolarisation():
	updateIntegration()
	updateIntegration()
	updateIntegration()
	RCP=readIntegration64bit('Subsystem1_ch2','Subsystem1_ch1')
	LCP=readIntegration64bit('Subsystem1_ch4','Subsystem1_ch3')
	U=readIntegration64bit('Subsystem1_ch8','Subsystem1_ch7')
	Q=readIntegration64bit('Subsystem1_ch6','Subsystem1_ch5')
	TRCP=readIntegration64bit('Subsystem1_ch10','Subsystem1_ch9')
	TLCP=readIntegration64bit('Subsystem1_ch12','Subsystem1_ch11')
	return RCP,LCP,U,Q,TRCP,TLCP
	

def writeCoffs(Coffs1R,Coffs1I,Coffs2R,Coffs2I,Coffs3R,Coffs3I,Coffs4R,Coffs4I,fpga):
	
	
	
	#now create the first set of coefficients for the real
	coffs1even=numpy.round(Coffs1R[::2])
	coffs1odd=numpy.round(Coffs1R[1::2])
	coffs2even=numpy.round(Coffs2R[::2])
	coffs2odd=numpy.round(Coffs2R[1::2])
	coffs3even=numpy.round(Coffs3R[::2])
	coffs3odd=numpy.round(Coffs3R[1::2])
	coffs4even=numpy.round(Coffs4R[::2])
	coffs4odd=numpy.round(Coffs4R[1::2])
	bramw(fpga,'amp_EQ0_coeff_real', coffs1even, 32)
	bramw(fpga,'amp_EQ1_coeff_real', coffs1odd, 32)
	bramw(fpga,'amp_EQ2_coeff_real', coffs2even, 32)
	bramw(fpga,'amp_EQ3_coeff_real', coffs2odd, 32)
	bramw(fpga,'amp_EQ4_coeff_real', coffs3even, 32)
	bramw(fpga,'amp_EQ5_coeff_real', coffs3odd, 32)
	bramw(fpga,'amp_EQ6_coeff_real', coffs4even, 32)
	bramw(fpga,'amp_EQ7_coeff_real', coffs4odd, 32)
	coffs1evenr=numpy.round(Coffs1R[::2])
	coffs1oddr=numpy.round(Coffs1R[1::2])
	coffs2evenr=numpy.round(Coffs2R[::2])
	coffs2oddr=numpy.round(Coffs2R[1::2])
	coffs3evenr=numpy.round(Coffs3R[::2])
	coffs3oddr=numpy.round(Coffs3R[1::2])
	coffs4evenr=numpy.round(Coffs4R[::2])
	coffs4oddr=numpy.round(Coffs4R[1::2])
	
	
	
	#now create the second set of coefficients for the imaginary
	coffs1even=numpy.round(Coffs1I[::2])
	coffs1odd=numpy.round(Coffs1I[1::2])
	coffs2even=numpy.round(Coffs2I[::2])
	coffs2odd=numpy.round(Coffs2I[1::2])
	coffs3even=numpy.round(Coffs3I[::2])
	coffs3odd=numpy.round(Coffs3I[1::2])
	coffs4even=numpy.round(Coffs4I[::2])
	coffs4odd=numpy.round(Coffs4I[1::2])
	
	bramw(fpga,'amp_EQ0_coeff_imag', coffs1even, 32)
	bramw(fpga,'amp_EQ1_coeff_imag', coffs1odd, 32)
	bramw(fpga,'amp_EQ2_coeff_imag', coffs2even, 32)
	bramw(fpga,'amp_EQ3_coeff_imag', coffs2odd, 32)
	bramw(fpga,'amp_EQ4_coeff_imag', coffs3even, 32)
	bramw(fpga,'amp_EQ5_coeff_imag', coffs3odd, 32)
	bramw(fpga,'amp_EQ6_coeff_imag', coffs4even, 32)
	bramw(fpga,'amp_EQ7_coeff_imag', coffs4odd, 32)
	
 	#first read the current coefficients
	b0r=bramr(fpga,'amp_EQ0_coeff_real',32)
	b1r=bramr(fpga,'amp_EQ1_coeff_real',32)
	b2r=bramr(fpga,'amp_EQ2_coeff_real',32)
	b3r=bramr(fpga,'amp_EQ3_coeff_real',32)
	b4r=bramr(fpga,'amp_EQ4_coeff_real',32)
	b5r=bramr(fpga,'amp_EQ5_coeff_real',32)
	b6r=bramr(fpga,'amp_EQ6_coeff_real',32)
	b7r=bramr(fpga,'amp_EQ7_coeff_real',32)

	b0i=bramr(fpga,'amp_EQ0_coeff_imag',32)
	b1i=bramr(fpga,'amp_EQ1_coeff_imag',32)
	b2i=bramr(fpga,'amp_EQ2_coeff_imag',32)
	b3i=bramr(fpga,'amp_EQ3_coeff_imag',32)
	b4i=bramr(fpga,'amp_EQ4_coeff_imag',32)
	b5i=bramr(fpga,'amp_EQ5_coeff_imag',32)
	b6i=bramr(fpga,'amp_EQ6_coeff_imag',32)
	b7i=bramr(fpga,'amp_EQ7_coeff_imag',32)
	
	Coffs1R[::2]=b0r
	Coffs1R[1::2]=b1r
	Coffs2R[::2]=b2r
	Coffs2R[1::2]=b3r
	Coffs3R[::2]=b4r
	Coffs3R[1::2]=b5r
	Coffs4R[::2]=b6r
	Coffs4R[1::2]=b7r
	
	Coffs1I[::2]=b0i
	Coffs1I[1::2]=b1i
	Coffs2I[::2]=b2i
	Coffs2I[1::2]=b3i
	Coffs3I[::2]=b4i
	Coffs3I[1::2]=b5i
	Coffs4I[::2]=b6i
	Coffs4I[1::2]=b7i
	numpy.savetxt('amp0real.txt',coffs1evenr,fmt="%6G");
	numpy.savetxt('amp1real.txt',coffs1oddr,fmt="%6G");
	numpy.savetxt('amp2real.txt',coffs2evenr,fmt="%6G");
	numpy.savetxt('amp3real.txt',coffs2oddr,fmt="%6G");
	numpy.savetxt('amp4real.txt',coffs3evenr,fmt="%6G");
	numpy.savetxt('amp5real.txt',coffs3oddr,fmt="%6G");
	numpy.savetxt('amp6real.txt',coffs4evenr,fmt="%6G");
	numpy.savetxt('amp7real.txt',coffs4oddr,fmt="%6G");
	numpy.savetxt('amp0imag.txt',coffs1even,fmt="%6G");
	numpy.savetxt('amp1imag.txt',coffs1odd,fmt="%6G");
	numpy.savetxt('amp2imag.txt',coffs2even,fmt="%6G");
	numpy.savetxt('amp3imag.txt',coffs2odd,fmt="%6G");
	numpy.savetxt('amp4imag.txt',coffs3even,fmt="%6G");
	numpy.savetxt('amp5imag.txt',coffs3odd,fmt="%6G");
	numpy.savetxt('amp6imag.txt',coffs4even,fmt="%6G");
	numpy.savetxt('amp7imag.txt',coffs4odd,fmt="%6G");
	
	
	print 'COFFS\n',Coffs1R,Coffs1I,Coffs2R,Coffs2I,Coffs3R,Coffs3I,Coffs4R,Coffs4I
	return Coffs1R,Coffs1I,Coffs2R,Coffs2I,Coffs3R,Coffs3I,Coffs4R,Coffs4I
	


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
        roachND = args[1]

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
		fpgaND = corr.katcp_wrapper.FpgaClient(roachND, katcp_port, timeout=10,logger=logger)
		time.sleep(1)

		if fpga.is_connected():
			print 'ok\n'
		else:
			print 'ERROR connecting to server %s on port %i.\n'%(roach,katcp_port)
			exit_fail()




		print '------------------------'
		print 'Programming FPGA...',
		if not opts.skip:
			fpga.progdev(bitstreampower)
			fpgaND.progdev(bitstreampower)
			print 'done'
		else:
			print 'Skipped.'

		print 'Configuring accumulation period...',
		fpga.write_int('acc_len',781250*2)
		#fpga.write_int('acc_len',1781250)
#		fpga.write_int('acc_len',1000000)
		#fpga.write_int('acc_len',500000)
		print 'done',str(opts.acc_len)

		print 'Resetting counters...',
		fpga.write_int('cnt_rst',1) 
		fpga.write_int('cnt_rst',0) 
		fpga.write_int('snap_we',0)
		fpga.write_int('snap_we',1)
		fpga.write_int('sync_en',1)
		fpga.write_int('snap_we',1)
		fpga.write_int('ctrl_sw',43690) #fft size
		fpga.write_int('acc_sync_delay',10) #fft size
		print 'done'
	

		Ch1onSum=numpy.zeros(64,dtype=numpy.uint64)
		Ch1offSum=numpy.zeros(64,dtype=numpy.uint64)
		Ch2onSum=numpy.zeros(64,dtype=numpy.uint64)
		Ch2offSum=numpy.zeros(64,dtype=numpy.uint64)
		Ch3onSum=numpy.zeros(64,dtype=numpy.uint64)
		Ch3offSum=numpy.zeros(64,dtype=numpy.uint64)
		Ch4onSum=numpy.zeros(64,dtype=numpy.uint64)
		Ch4offSum=numpy.zeros(64,dtype=numpy.uint64)
		initialiseCoefficients(fpga)#set coefficients to 1 and zeros
		
		
		time.sleep(0.5)

		intNumber=3

		
		


except KeyboardInterrupt:
    exit_clean()
except:
    exit_fail()

#exit_clean()

