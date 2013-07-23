#!/usr/bin/env python
'''
Code to query the cbass roach in polarisation
You need to have KATCP and CORR installed. Get them from http://pypi.python.org/pypi/katcp and http://casper.berkeley.edu/svn/trunk/projects/packetized_correlator/corr-0.4.0/

\nAuthor: Charles Copley January 2012
'''

#TODO: add support for ADC histogram plotting.
#TODO: add support for determining ADC input level 

import corr,time,numpy,struct,sys,logging,pylab,serial
from scipy.interpolate import interp1d

bitstreampol = 'cbassrx_6mara_pol_2012_Mar_06_1041'

bitstreampol = bitstreampol+'.bof'
katcp_port=7147
sleep_time=0.005
desiredPower=1e14


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
    even=numpy.array(ac,dtype=numpy.int64)
    
    aa=numpy.array(oddarrayLSB,dtype=numpy.uint64)
    ab=numpy.array(oddarrayMSB,dtype=numpy.uint64)
    ab=ab<<32
    ac=aa|ab
    odd=numpy.array(ac,dtype=numpy.int64)

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

def plotSpectrum4Linear(spectrum1On,spectrum2On,spectrum3On,spectrum4On,filename=""):
		pylab.ion() 	
		pylab.figure(0) 	
		pylab.clf() 
		pylab.grid()
		pylab.subplot(221)
		pylab.plot((spectrum1On[5:63]),'r')
		#pylab.plot((spectrum1Off[5:63]))
		pylab.title('Channel1')
		pylab.subplot(222)
		pylab.title('Channel2')
		pylab.plot((spectrum2On[5:63]),'r')
		#pylab.plot((spectrum2Off[5:63]))
		pylab.subplot(223)
		pylab.title('Channel3')
		pylab.plot((spectrum3On[5:63]),'r')
		#pylab.plot(10*numpy.log10(spectrum3Off[5:63]))
		pylab.subplot(224)
		pylab.title('Channel4')
		pylab.plot((spectrum4On[5:63]),'r')
		#pylab.plot(10*numpy.log10(spectrum4Off[5:63]))
		pylab.draw()
		pylab.savefig(filename)
def plotSpectrum4(spectrum1On,spectrum2On,spectrum3On,spectrum4On,spectrum1Off,spectrum2Off,spectrum3Off,spectrum4Off,filename=""):
		pylab.ion() 	
		pylab.figure(0) 	
		pylab.clf() 
		pylab.grid()
		pylab.subplot(221)
		pylab.plot(10*numpy.log10(spectrum1On[5:63]),'r')
		pylab.plot(10*numpy.log10(spectrum1Off[5:63]))
		pylab.title('Channel1')
		pylab.subplot(222)
		pylab.title('Channel2')
		pylab.plot(10*numpy.log10(spectrum2On[5:63]),'r')
		pylab.plot(10*numpy.log10(spectrum2Off[5:63]))
		pylab.subplot(223)
		pylab.title('Channel3')
		pylab.plot(10*numpy.log10(spectrum3On[5:63]),'r')
		pylab.plot(10*numpy.log10(spectrum3Off[5:63]))
		pylab.subplot(224)
		pylab.title('Channel4')
		pylab.plot(10*numpy.log10(spectrum4On[5:63]),'r')
		pylab.plot(10*numpy.log10(spectrum4Off[5:63]))
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

def plotTheta(thetaR,thetaL,filename=""):
		pylab.ion() 	
		pylab.figure(0) 	
		pylab.clf() 
		pylab.grid()
		pylab.subplot(121)
		pylab.plot(numpy.rad2deg(thetaR),'r')
		pylab.title('thetaRCP')
		pylab.subplot(122)
		pylab.title('thetaLCP')
		pylab.plot(numpy.rad2deg(thetaL),'r')
		pylab.draw()
		pylab.savefig(filename)



    
def bramr(fpga, bname, samples=1024):
    bramvals=fpga.read(bname,samples*4,0)  	    
    string = '>'+str(samples)+'i'
    b_0=struct.unpack(string,bramvals)
    data=numpy.array(b_0)	
    return data

def updateIntegration():    
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

def diodeOn():
    fpga.write_int('gpio_set4',1)
    #fpga.write_int('gpio1_set',1)
    return 1

def diodeOff():
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
   
def initialiseCoefficients():
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

	[Coffs1R,Coffs1I,Coffs2R,Coffs2I,Coffs3R,Coffs3I,Coffs4R,Coffs4I]=writeCoffs(Coffs1R,Coffs1I,Coffs2R,Coffs2I,Coffs3R,Coffs3I,Coffs4R,Coffs4I)
	
	return Coffs1R,Coffs1I,Coffs2R,Coffs2I,Coffs3R,Coffs3I,Coffs4R,Coffs4I
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

def calculateThetaStep2(QOn,QOff,UOn,UOff):
	dQ=QOn-QOff
	dU=UOn-UOff
	argP=dQ/dU
	thetaP=numpy.arctan(argP)
#	print 'theta1111',numpy.rad2deg(thetaP)
	return thetaP

def calculateThetaStep1(IOn,IOff,loadOn,loadOff):
	dL=IOn-IOff

	dRef=loadOn-loadOff
	#print dL,dR
	#print dLRef,dRRef
	t1=dL/dRef-1.
	t2=dL/dRef+1.
	a=t1/t2
	a=a.clip(min=-1,max=+1)
	theta=numpy.arccos(a)
        thetaSmooth= savitzky_golay(theta,15,10)
	print thetaSmooth	
	#theta=numpy.nan_to_num(theta)
	return theta

def rotateVector(CoffsR,CoffsI,theta):
	
	CoffsComplex=numpy.zeros(64,complex)


	
	CoffsComplex = CoffsR+1j*CoffsI
	

	
	CoffsComplex=CoffsComplex*numpy.exp(1j*theta)
	
	CoffsR=numpy.real(CoffsComplex)
	CoffsI=numpy.imag(CoffsComplex)

	print 'real',numpy.real(CoffsComplex),'imag',numpy.imag(CoffsComplex)
	return [CoffsR,CoffsI]
 
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
	
def readCoffs():
	coffs1evenR=numpy.loadtxt('amp0real.txt',numpy.uint)
	coffs2evenR=numpy.loadtxt('amp2real.txt',numpy.uint)
	coffs3evenR=numpy.loadtxt('amp4real.txt',numpy.uint)
	coffs4evenR=numpy.loadtxt('amp6real.txt',numpy.uint)
	coffs1oddR=numpy.loadtxt('amp1real.txt',numpy.uint)
	coffs2oddR=numpy.loadtxt('amp3real.txt',numpy.uint)
	coffs3oddR=numpy.loadtxt('amp5real.txt',numpy.uint)
	coffs4oddR=numpy.loadtxt('amp7real.txt',numpy.uint)
	coffs1evenI=numpy.loadtxt('amp0imag.txt',numpy.uint)
	coffs2evenI=numpy.loadtxt('amp2imag.txt',numpy.uint)
	coffs3evenI=numpy.loadtxt('amp4imag.txt',numpy.uint)
	coffs4evenI=numpy.loadtxt('amp6imag.txt',numpy.uint)
	coffs1oddI=numpy.loadtxt('amp1imag.txt',numpy.uint)
	coffs2oddI=numpy.loadtxt('amp3imag.txt',numpy.uint)
	coffs3oddI=numpy.loadtxt('amp5imag.txt',numpy.uint)
	coffs4oddI=numpy.loadtxt('amp7imag.txt',numpy.uint)
	
        bramw(fpga,'amp_EQ0_coeff_real', coffs1evenR, 32)
	bramw(fpga,'amp_EQ1_coeff_real', coffs1oddR, 32)
	bramw(fpga,'amp_EQ2_coeff_real', coffs2evenR, 32)
	bramw(fpga,'amp_EQ3_coeff_real', coffs2oddR, 32)
	bramw(fpga,'amp_EQ4_coeff_real', coffs3evenR, 32)
	bramw(fpga,'amp_EQ5_coeff_real', coffs3oddR, 32)
	bramw(fpga,'amp_EQ6_coeff_real', coffs4evenR, 32)
	bramw(fpga,'amp_EQ7_coeff_real', coffs4oddR, 32)
	bramw(fpga,'amp_EQ0_coeff_imag', coffs1evenI, 32)
	bramw(fpga,'amp_EQ1_coeff_imag', coffs1oddI, 32)
	bramw(fpga,'amp_EQ2_coeff_imag', coffs2evenI, 32)
	bramw(fpga,'amp_EQ3_coeff_imag', coffs2oddI, 32)
	bramw(fpga,'amp_EQ4_coeff_imag', coffs3evenI, 32)
	bramw(fpga,'amp_EQ5_coeff_imag', coffs3oddI, 32)
	bramw(fpga,'amp_EQ6_coeff_imag', coffs4evenI, 32)
	bramw(fpga,'amp_EQ7_coeff_imag', coffs4oddI, 32)

	coffs1R=numpy.zeros(64)
	coffs2R=numpy.zeros(64)
	coffs3R=numpy.zeros(64)
	coffs4R=numpy.zeros(64)
	coffs1I=numpy.zeros(64)
	coffs2I=numpy.zeros(64)
	coffs3I=numpy.zeros(64)
	coffs4I=numpy.zeros(64)

	coffs1R[::2]=coffs1evenR
	coffs2R[::2]=coffs2evenR
	coffs3R[::2]=coffs3evenR
	coffs4R[::2]=coffs4evenR
	coffs1R[1::2]=coffs1oddR
	coffs2R[1::2]=coffs2oddR
	coffs3R[1::2]=coffs3oddR
	coffs4R[1::2]=coffs4oddR
	coffs1I[::2]=coffs1evenI
	coffs2I[::2]=coffs2evenI
	coffs3I[::2]=coffs3evenI
	coffs4I[::2]=coffs4evenI
	coffs1I[1::2]=coffs1oddI
	coffs2I[1::2]=coffs2oddI
	coffs3I[1::2]=coffs3oddI
	coffs4I[1::2]=coffs4oddI

	return [coffs1R,coffs2R,coffs3R,coffs4R,coffs1I,coffs2I,coffs3I,coffs4I]

def writeCoffs(Coffs1R,Coffs1I,Coffs2R,Coffs2I,Coffs3R,Coffs3I,Coffs4R,Coffs4I):
	
	
	
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




		print '------------------------'
		print 'Programming FPGA...',
		if not opts.skip:
			fpga.progdev(bitstreampol)
			print 'done'
		else:
			print 'Skipped.'

		print 'Configuring accumulation period...',
		fpga.write_int('acc_len',78125)
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
	

		[c1R,c2R,c3R,c4R,c1I,c2I,c3I,c4I]=readCoffs()	
		print c2R[5],c2I[5]
		time.sleep(1)
	
		updateIntegration()
		updateIntegration()
		updateIntegration()
		updateIntegration()

		C1On=readIntegration64bit('Subsystem1_ch1','Subsystem1_ch2')
		C2On=readIntegration64bit('Subsystem1_ch3','Subsystem1_ch4')
		C3On=readIntegration64bit('Subsystem1_ch9','Subsystem1_ch10')
		C4On=readIntegration64bit('Subsystem1_ch11','Subsystem1_ch12')
		#Channel3On=readIntegration64bit('Subsystem1_ch9','Subsystem1_ch10')
		#Channel4On=readIntegration64bit('Subsystem1_ch11','Subsystem1_ch12')
		#Channel3On=Channel1On
		#Channel4On=Channel2On
		Channel1On=C1On
		Channel2On=C2On
		Channel3On=C3On
		Channel4On=C4On
		c1ondB=10*numpy.log10(C1On)
		c2ondB=10*numpy.log10(C2On)
		c3ondB=10*numpy.log10(C3On)
		c4ondB=10*numpy.log10(C4On)

#		print c1ondB
#		print c2ondB
#		print c3ondB
#		print c4ondB

	
		diodeOff()
		time.sleep(0.5)
		updateIntegration()
		updateIntegration()
		updateIntegration()
		

		Channel1Off=readIntegration64bit('Subsystem1_ch1','Subsystem1_ch2')
		Channel2Off=readIntegration64bit('Subsystem1_ch3','Subsystem1_ch4')
		Channel3Off=readIntegration64bit('Subsystem1_ch9','Subsystem1_ch10')
		Channel4Off=readIntegration64bit('Subsystem1_ch11','Subsystem1_ch12')
		c1offdB=10*numpy.log10(Channel1Off)
		c2offdB=10*numpy.log10(Channel2Off)
		c3offdB=10*numpy.log10(Channel3Off)
		c4offdB=10*numpy.log10(Channel4Off)
	#	Channel3Off=Channel1Off
	#	Channel4Off=Channel2Off

#		c1diff=10*numpy.log10(Channel1On)-10*numpy.log10*(Channel1Off)
#		c2diff=10*numpy.log10(Channel2On)-10*numpy.log10*(Channel2Off)
#		c3diff=10*numpy.log10(Channel3On)-10*numpy.log10*(Channel3Off)
#		c4diff=10*numpy.log10(Channel4On)-10*numpy.log10*(Channel4Off)

	#	print numpy.mean(c1ondB[10]-c3ondB[10])
	#	print numpy.mean(c2ondB[10]-c4ondB[10])
	#	print numpy.mean(c3ondB[10]-c3offdB[10])
	#	print numpy.mean(c4ondB[10]-c4offdB[10])
		plotSpectrum4(Channel1On,Channel2On,Channel3On,Channel4On,Channel1Off,Channel2Off,Channel3Off,Channel4Off,"OnChannel")
		i=0
		j=0
		test=numpy.ones(64)
		incr=0.05
		theta=numpy.ones(64)
		theta=theta*incr
		print 'j = ',j

		RRonSum=numpy.zeros(64,dtype=numpy.uint64)
		RRoffSum=numpy.zeros(64,dtype=numpy.uint64)
		LLonSum=numpy.zeros(64,dtype=numpy.uint64)
		LLoffSum=numpy.zeros(64,dtype=numpy.uint64)
		RloadonSum=numpy.zeros(64,dtype=numpy.uint64)
		RloadoffSum=numpy.zeros(64,dtype=numpy.uint64)
		LloadonSum=numpy.zeros(64,dtype=numpy.uint64)
		LloadoffSum=numpy.zeros(64,dtype=numpy.uint64)
		intNum=2
		diodeOn()
		time.sleep(0.1)

		updateIntegration()
		updateIntegration()
		updateIntegration()
		updateIntegration()

		RROn=readIntegration64bit('Subsystem1_ch1','Subsystem1_ch2')
		LLOn=readIntegration64bit('Subsystem1_ch3','Subsystem1_ch4')
		QOn=readIntegration64bit('Subsystem1_ch5','Subsystem1_ch6')
		UOn=readIntegration64bit('Subsystem1_ch7','Subsystem1_ch8')
		RloadOn=readIntegration64bit('Subsystem1_ch9','Subsystem1_ch10')
		LloadOn=readIntegration64bit('Subsystem1_ch11','Subsystem1_ch12')


	
		diodeOff()
		time.sleep(0.1)
		updateIntegration()
		updateIntegration()
		updateIntegration()
		

		RROff=readIntegration64bit('Subsystem1_ch1','Subsystem1_ch2')
		LLOff=readIntegration64bit('Subsystem1_ch3','Subsystem1_ch4')
		QOff=readIntegration64bit('Subsystem1_ch5','Subsystem1_ch6')
		UOff=readIntegration64bit('Subsystem1_ch7','Subsystem1_ch8')
		RloadOff=readIntegration64bit('Subsystem1_ch9','Subsystem1_ch10')
		LloadOff=readIntegration64bit('Subsystem1_ch11','Subsystem1_ch12')
		print '////////////////////////////////////'
		RRonSum=RRonSum+RROn
		RRoffSum=RRoffSum+RROff
		LLonSum=LLonSum+LLOn
		LLoffSum=LLoffSum+LLOff
		RloadonSum=RloadonSum+RloadOn
		RloadoffSum=RloadoffSum+RloadOff
		LloadonSum=LloadonSum+LloadOn
		LloadoffSum=LloadoffSum+LloadOff
		filename="thetaValues_"+str(i)
		thetaR=calculateThetaStep1(RROn,RROff,RloadOn,RloadOff)
		thetaL=calculateThetaStep1(LLOn,LLOff,LloadOn,LloadOff)
		plotTheta(thetaR,thetaL,filename)
		[c2R,c2I]=rotateVector(c2R,c2I,thetaR)	
		[c4R,c4I]=rotateVector(c4R,c4I,thetaL)
		fileplot1='PolChannels_'+str(i)	
		fileplot2='PolChannels2_'+str(i)	
		fileplot3='PolChannels3_'+str(i)	
		plotSpectrum4(RROn,LLOn,RloadOn,LloadOn,RROff,LLOff,RloadOff,LloadOff,fileplot1)
		plotSpectrum4Linear(QOn,UOn,QOff,UOff,fileplot2)
		
		[Coffs1R,Coffs1I,Coffs2R,Coffs2I,Coffs3R,Coffs3I,Coffs4R,Coffs4I]=writeCoffs(c1R,c1I,c2R,c2I,c3R,c3I,c4R,c4I)

		diodeOn()
		time.sleep(0.1)

		updateIntegration()
		updateIntegration()
		updateIntegration()
		updateIntegration()

		RROn=readIntegration64bit('Subsystem1_ch1','Subsystem1_ch2')
		LLOn=readIntegration64bit('Subsystem1_ch3','Subsystem1_ch4')
		QOn=readIntegration64bit('Subsystem1_ch5','Subsystem1_ch6')
		UOn=readIntegration64bit('Subsystem1_ch7','Subsystem1_ch8')
		RloadOn=readIntegration64bit('Subsystem1_ch9','Subsystem1_ch10')
		LloadOn=readIntegration64bit('Subsystem1_ch11','Subsystem1_ch12')


	
		diodeOff()
		time.sleep(0.1)
		updateIntegration()
		updateIntegration()
		updateIntegration()
		

		RROff=readIntegration64bit('Subsystem1_ch1','Subsystem1_ch2')
		LLOff=readIntegration64bit('Subsystem1_ch3','Subsystem1_ch4')
		QOff=readIntegration64bit('Subsystem1_ch5','Subsystem1_ch6')
		UOff=readIntegration64bit('Subsystem1_ch7','Subsystem1_ch8')
		RloadOff=readIntegration64bit('Subsystem1_ch9','Subsystem1_ch10')
		LloadOff=readIntegration64bit('Subsystem1_ch11','Subsystem1_ch12')
			
		plotSpectrum4(RROn,LLOn,RloadOn,LloadOn,RROff,LLOff,RloadOff,LloadOff,'polafinala')

		thetaP=calculateThetaStep2(QOn,QOff,UOn,UOff)
		

		plotTheta(thetaP,thetaP,"thetaPvalues")
		time0 = time.time()
		diodeOff()
		diodeStatus=1
		filename='roachPol.txt'
		while(1):
			updateIntegration()
			RR=readIntegration64bit('Subsystem1_ch1','Subsystem1_ch2')
			LL=readIntegration64bit('Subsystem1_ch3','Subsystem1_ch4')
			Rload=readIntegration64bit('Subsystem1_ch9','Subsystem1_ch10')
			Lload=readIntegration64bit('Subsystem1_ch11','Subsystem1_ch12')
			timeHere = time.time()
			diffTime= timeHere-time0
			if(diffTime>10):
				time0=timeHere
				if(diodeStatus==0):
					diodeOn()
					diodeStatus=1
					print 'Noise Diode On'
				elif(diodeStatus==1):
					diodeOff()
					diodeStatus=0
					print 'Noise Diode Off'
			RRdB= 10*numpy.log10(numpy.mean(RR[10:55],dtype=numpy.uint64))
			LLdB= 10*numpy.log10(numpy.mean(LL[10:55],dtype=numpy.uint64))
			RloaddB= 10*numpy.log10(numpy.mean(Rload[10:55],dtype=numpy.uint64))
			LloaddB= 10*numpy.log10(numpy.mean(Lload[10:55],dtype=numpy.uint64))
			print RRdB,LLdB,RloaddB,LloaddB
			c="%s,%s,%s,%s\n" % (RRdB,LLdB,RloaddB,LloaddB)
			#c="%s,%d\n" %(out1,integration)
			timeStamp = fpga.read_uint('Subsystem1_timeStamp')
			f = open(filename, 'a')
			#time.sleep(0.1)	
			f.write(c)
			f.close()

except KeyboardInterrupt:
    exit_clean()
except:
    exit_fail()

exit_clean()

