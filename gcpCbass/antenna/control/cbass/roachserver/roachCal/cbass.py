#!/usr/bin/env python
'''
C-BASS module for interacting with the receiver through python
You need to have KATCP and CORR installed. Get them from http://pypi.python.org/pypi/katcp and http://casper.berkeley.edu/svn/trunk/projects/packetized_correlator/corr-0.4.0/

\nAuthor: Charles Copley, November 20122.
'''

#TODO: add support for ADC histogram plotting.
#TODO: add support for determining ADC input level 

import corr,time,numpy,struct,sys,logging,pylab,serial,os

def exit_fail(fpga):
    print 'FAILURE DETECTED. Log entries:\n',lh.printMessages()
    try:
        fpga.stop()
    except: pass
    raise
    exit()

def exit_clean(fpga):
    try:
        fpga.stop()
    except: pass
    exit()
    
def updateIntegration(fpga):    
#function that get's the next integration. It looks at the Subsystem1_readAccumulation register which keeps a count of the number of accumulations. When that increases it releases the function
		prev_integration = fpga.read_uint('Subsystem1_readAccumulation')
		current_integration = fpga.read_uint('Subsystem1_readAccumulation')
		diff=current_integration - prev_integration

		while diff==0:
				current_integration = fpga.read_uint('Subsystem1_readAccumulation')
				diff=current_integration - prev_integration
		
		if diff > 1:
				print 'WARN: We lost %i integrations!'%(current_integration - prev_integration)
		return current_integration


	
	



def getSnapBlock(snap=""):
    snapctrl='fft_'+snap+'_ctrl'
    snapaddr='fft_'+snap+'_addr'
    snapbram='fft_'+snap+'_bram'

    fpga.write_int(snapctrl,0)	
    fpga.write_int(snapctrl,1)	
    while(fpga.read_uint(snapaddr)<100):
	time.sleep(0.01)
    snapreturn=struct.unpack(fftstringUnsigned,fpga.read(snapbram,fftreal*4,0))
    time.sleep(0.01)

    return snapreturn

def getADCSnapBlock(snap=""):
    snapctrl='fft_'+snap+'_ctrl'
    snapaddr='fft_'+snap+'_addr'
    snapbram='fft_'+snap+'_bram'
    fpga.write_int(snapctrl,0)
    fpga.write_int(snapctrl,5)
    fpga.write_int(snapctrl,7)	
    while(fpga.read_uint(snapaddr)<100):
	time.sleep(0.01)
    snapreturn=struct.unpack(ADCstringSigned,fpga.read(snapbram,256,0))
    time.sleep(sleep_time)
    return snapreturn


class roach:
	class status:
		noiseDiode=0
		readoutType=0 #defined here as 0 for powr readout and 1 for polarisation readout
		accDelay=0

	class Coefficients:
		b0r=numpy.zeros(32)
		b1r=numpy.zeros(32)
		b2r=numpy.zeros(32)
		b3r=numpy.zeros(32)
		b4r=numpy.zeros(32)
		b5r=numpy.zeros(32)
		b6r=numpy.zeros(32)
		b7r=numpy.zeros(32)
		b0i=numpy.zeros(32)
		b1i=numpy.zeros(32)
		b2i=numpy.zeros(32)
		b3i=numpy.zeros(32)
		b4i=numpy.zeros(32)
		b5i=numpy.zeros(32)
		b6i=numpy.zeros(32)
		b7i=numpy.zeros(32)
		Coeffs1 = numpy.zeros(64,complex)
		Coeffs2 = numpy.zeros(64,complex)
		Coeffs3 = numpy.zeros(64,complex)
		Coeffs4 = numpy.zeros(64,complex)
		


	def updatePower(self):
	#method to get the power output
		Ch1 = self.readIntegration64bit('Subsystem1_ch1','Subsystem1_ch2')
		Ch2 = self.readIntegration64bit('Subsystem1_ch3','Subsystem1_ch4')
		Ch3 = self.readIntegration64bit('Subsystem1_ch5','Subsystem1_ch6')
		Ch4 = self.readIntegration64bit('Subsystem1_ch7','Subsystem1_ch8')
		return [Ch1,Ch2,Ch3,Ch4]

	def updatePolarisation(self):
	#method to return the polarisation#note the ordering of the output
		RR = self.readIntegration64bit('Subsystem1_ch1','Subsystem1_ch2') 
		Rload= self.readIntegration64bit('Subsystem1_ch9','Subsystem1_ch10') 
		LL = self.readIntegration64bit('Subsystem1_ch3','Subsystem1_ch4')  
		Lload= self.readIntegration64bit('Subsystem1_ch11','Subsystem1_ch12')
		Q =  self.readIntegration64bitSigned('Subsystem1_ch5','Subsystem1_ch6') 
		U = self.readIntegration64bitSigned('Subsystem1_ch7','Subsystem1_ch8') 
		return [RR,Rload,LL,Lload,Q,U]
		
	def readIntegration64bit(self,nameeven="",nameodd=""):

	    evenBram=nameeven+'_msb'
	    oddBram=nameodd+'_msb'
	    evenBramlsb=nameeven+'_lsb'
	    oddBramlsb=nameodd+'_lsb'

	    evenMSB=self.fpga.read(evenBram,32*4,0)
	    oddMSB=self.fpga.read(oddBram,32*4,0)
	    evenLSB=self.fpga.read(evenBramlsb,32*4,0)
	    oddLSB=self.fpga.read(oddBramlsb,32*4,0)

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

	def readIntegration64bitSigned(self,nameeven="",nameodd=""):

	    evenBram=nameeven+'_msb'
	    oddBram=nameodd+'_msb'
	    evenBramlsb=nameeven+'_lsb'
	    oddBramlsb=nameodd+'_lsb'

	    evenMSB=self.fpga.read(evenBram,32*4,0)
	    oddMSB=self.fpga.read(oddBram,32*4,0)
	    evenLSB=self.fpga.read(evenBramlsb,32*4,0)
	    oddLSB=self.fpga.read(oddBramlsb,32*4,0)

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
	    aa=numpy.array(evenarrayLSB,dtype=numpy.int64)
	    ab=numpy.array(evenarrayMSB,dtype=numpy.int64)
	    ab=ab<<32
	    ac=aa|ab
	    even=numpy.array(ac,dtype=numpy.int64)

	    aa=numpy.array(oddarrayLSB,dtype=numpy.int64)
	    ab=numpy.array(oddarrayMSB,dtype=numpy.int64)
	    ab=ab<<32
	    ac=aa|ab
	    odd=numpy.array(ac,dtype=numpy.int64)

	    #print 'even',numpy.mean(numpy.array(even)) 
	    #print 'odd', numpy.mean(numpy.array(odd)) 
	    fullSpectrum[::2]=numpy.array(even)
	    fullSpectrum[1::2]=numpy.array(odd)
	    return fullSpectrum		
			
		
	def instantiateRoach(self,roachName,progdevFile,katcp_port,accDelay,accLength):
	#this instantiates the Roach
		print('Connecting to server %s on port %i................. '%(roachName,katcp_port)),
		try:	
			if self.fpga.is_connected()==True:
				try:
					print 'Killing current fpga instance'
					self.fpga.stop()
				except:
					print 'No current process running... continuing'
		except:
			print 'Initialising the fpga instance'

		self.fpga = corr.katcp_wrapper.FpgaClient(roachName, katcp_port, timeout=10)
		time.sleep(0.1)
		self.fpga.progdev(progdevFile)
		self.fpga.write_int('acc_len',accLength)

		print '\n......Resetting counters...',
		self.fpga.write_int('cnt_rst',1)
		self.fpga.write_int('cnt_rst',0)
		self.fpga.write_int('snap_we',0)
		self.fpga.write_int('snap_we',1)
		self.fpga.write_int('sync_en',1)
		self.fpga.write_int('snap_we',1)
		self.fpga.write_int('ctrl_sw',43690) #fft size
		self.fpga.write_int('acc_sync_delay',accDelay) #fft size
		print('\n Successfully connected to server %s on port %i... '%(roachName,katcp_port)),
		self.status.accDelay=accDelay #store the value used to align the coefficients

	def readCoefficientsFromDisk(self,roachName):
		#the method reads the data stored in (for example) pumba/amp0real.txt ... to the Roach
		b0r = numpy.loadtxt(roachName+'/amp0real.txt');
		b1r = numpy.loadtxt(roachName+'/amp1real.txt');
		b2r = numpy.loadtxt(roachName+'/amp2real.txt');
		b3r = numpy.loadtxt(roachName+'/amp3real.txt');
		b4r = numpy.loadtxt(roachName+'/amp4real.txt');
		b5r = numpy.loadtxt(roachName+'/amp5real.txt');
		b6r = numpy.loadtxt(roachName+'/amp6real.txt');
		b7r = numpy.loadtxt(roachName+'/amp7real.txt');
		b0i = numpy.loadtxt(roachName+'/amp0imag.txt');
		b1i = numpy.loadtxt(roachName+'/amp1imag.txt');
		b2i = numpy.loadtxt(roachName+'/amp2imag.txt');
		b3i = numpy.loadtxt(roachName+'/amp3imag.txt');
		b4i = numpy.loadtxt(roachName+'/amp4imag.txt');
		b5i = numpy.loadtxt(roachName+'/amp5imag.txt');
		b6i = numpy.loadtxt(roachName+'/amp6imag.txt');
		b7i = numpy.loadtxt(roachName+'/amp7imag.txt');
		Coffs1 = numpy.zeros(64,complex)	
		Coffs2 = numpy.zeros(64,complex)	
		Coffs3 = numpy.zeros(64,complex)	
		Coffs4 = numpy.zeros(64,complex)	
	
		Coffs1[::2]=b0r + 1j*b0i
		Coffs1[1::2]=b1r + 1j*b1i
		Coffs2[::2]=b2r + 1j*b2i
		Coffs2[1::2]=b3r + 1j*b3i
		Coffs3[::2]=b4r + 1j*b4i
		Coffs3[1::2]=b5r + 1j*b5i
		Coffs4[::2]=b6r + 1j*b6i
		Coffs4[1::2]=b7r + 1j*b7i
		self.writeCoefficients(Coffs1,Coffs2,Coffs3,Coffs4)
		return [Coffs1,Coffs2,Coffs3,Coffs4]	

	def saveCoefficientstoDisk(self,roachName):
		#the method saves the data stored in the Roach to (for example) pumba/amp0real.txt ... to disk
		#try:
		#	os.mkdir(roachName)
		#except: 
		#	print 'failed to mkdir- might exist already??'
	
		b0r=self.bramr('amp_EQ0_coeff_real',32)
		b1r=self.bramr('amp_EQ1_coeff_real',32)
		b2r=self.bramr('amp_EQ2_coeff_real',32)
		b3r=self.bramr('amp_EQ3_coeff_real',32)
		b4r=self.bramr('amp_EQ4_coeff_real',32)
		b5r=self.bramr('amp_EQ5_coeff_real',32)
		b6r=self.bramr('amp_EQ6_coeff_real',32)
		b7r=self.bramr('amp_EQ7_coeff_real',32)
		
		b0i=self.bramr('amp_EQ0_coeff_imag',32)
		b1i=self.bramr('amp_EQ1_coeff_imag',32)
		b2i=self.bramr('amp_EQ2_coeff_imag',32)
		b3i=self.bramr('amp_EQ3_coeff_imag',32)
		b4i=self.bramr('amp_EQ4_coeff_imag',32)
		b5i=self.bramr('amp_EQ5_coeff_imag',32)
		b6i=self.bramr('amp_EQ6_coeff_imag',32)
		b7i=self.bramr('amp_EQ7_coeff_imag',32)


		numpy.savetxt(roachName+'/amp0real.txt',b0r,fmt="%6G");
		numpy.savetxt(roachName+'/amp1real.txt',b1r,fmt="%6G");
		numpy.savetxt(roachName+'/amp2real.txt',b2r,fmt="%6G");
		numpy.savetxt(roachName+'/amp3real.txt',b3r,fmt="%6G");
		numpy.savetxt(roachName+'/amp4real.txt',b4r,fmt="%6G");
		numpy.savetxt(roachName+'/amp5real.txt',b5r,fmt="%6G");
		numpy.savetxt(roachName+'/amp6real.txt',b6r,fmt="%6G");
		numpy.savetxt(roachName+'/amp7real.txt',b7r,fmt="%6G");
		numpy.savetxt(roachName+'/amp0imag.txt',b0i,fmt="%6G");
		numpy.savetxt(roachName+'/amp1imag.txt',b1i,fmt="%6G");
		numpy.savetxt(roachName+'/amp2imag.txt',b2i,fmt="%6G");
		numpy.savetxt(roachName+'/amp3imag.txt',b3i,fmt="%6G");
		numpy.savetxt(roachName+'/amp4imag.txt',b4i,fmt="%6G");
		numpy.savetxt(roachName+'/amp5imag.txt',b5i,fmt="%6G");
		numpy.savetxt(roachName+'/amp6imag.txt',b6i,fmt="%6G");
		numpy.savetxt(roachName+'/amp7imag.txt',b7i,fmt="%6G");


	def writeCoefficients(self,Coffs1,Coffs2,Coffs3,Coffs4):
		#this method writes the coefficients defined by Coffs1,Coffs2 etc. to the Roach
		coffs1evenr=numpy.round(numpy.real(Coffs1[::2]))
		coffs1oddr=numpy.round(numpy.real(Coffs1[1::2]))
		coffs2evenr=numpy.round(numpy.real(Coffs2[::2]))
		coffs2oddr=numpy.round(numpy.real(Coffs2[1::2]))
		coffs3evenr=numpy.round(numpy.real(Coffs3[::2]))
		coffs3oddr=numpy.round(numpy.real(Coffs3[1::2]))
		coffs4evenr=numpy.round(numpy.real(Coffs4[::2]))
		coffs4oddr=numpy.round(numpy.real(Coffs4[1::2]))

		coffs1eveni=numpy.round(numpy.imag(Coffs1[::2]))
		coffs1oddi=numpy.round(numpy.imag(Coffs1[1::2]))
		coffs2eveni=numpy.round(numpy.imag(Coffs2[::2]))
		coffs2oddi=numpy.round(numpy.imag(Coffs2[1::2]))
		coffs3eveni=numpy.round(numpy.imag(Coffs3[::2]))
		coffs3oddi=numpy.round(numpy.imag(Coffs3[1::2]))
		coffs4eveni=numpy.round(numpy.imag(Coffs4[::2]))
		coffs4oddi=numpy.round(numpy.imag(Coffs4[1::2]))
		
		self.bramw('amp_EQ0_coeff_real', coffs1evenr, 32)
		self.bramw('amp_EQ1_coeff_real', coffs1oddr, 32)
		self.bramw('amp_EQ2_coeff_real', coffs2evenr, 32)
		self.bramw('amp_EQ3_coeff_real', coffs2oddr, 32)
		self.bramw('amp_EQ4_coeff_real', coffs3evenr, 32)
		self.bramw('amp_EQ5_coeff_real', coffs3oddr, 32)
		self.bramw('amp_EQ6_coeff_real', coffs4evenr, 32)
		self.bramw('amp_EQ7_coeff_real', coffs4oddr, 32)
		self.bramw('amp_EQ0_coeff_imag', coffs1eveni, 32)
		self.bramw('amp_EQ1_coeff_imag', coffs1oddi, 32)
		self.bramw('amp_EQ2_coeff_imag', coffs2eveni, 32)
		self.bramw('amp_EQ3_coeff_imag', coffs2oddi, 32)
		self.bramw('amp_EQ4_coeff_imag', coffs3eveni, 32)
		self.bramw('amp_EQ5_coeff_imag', coffs3oddi, 32)
		self.bramw('amp_EQ6_coeff_imag', coffs4eveni, 32)
		self.bramw('amp_EQ7_coeff_imag', coffs4oddi, 32)

		self.Coefficients.b0r=self.bramr('amp_EQ0_coeff_real',32)
		self.Coefficients.b1r=self.bramr('amp_EQ1_coeff_real',32)
		self.Coefficients.b2r=self.bramr('amp_EQ2_coeff_real',32)
		self.Coefficients.b3r=self.bramr('amp_EQ3_coeff_real',32)
		self.Coefficients.b4r=self.bramr('amp_EQ4_coeff_real',32)
		self.Coefficients.b5r=self.bramr('amp_EQ5_coeff_real',32)
		self.Coefficients.b6r=self.bramr('amp_EQ6_coeff_real',32)
		self.Coefficients.b7r=self.bramr('amp_EQ7_coeff_real',32)
		
		self.Coefficients.b0i=self.bramr('amp_EQ0_coeff_imag',32)
		self.Coefficients.b1i=self.bramr('amp_EQ1_coeff_imag',32)
		self.Coefficients.b2i=self.bramr('amp_EQ2_coeff_imag',32)
		self.Coefficients.b3i=self.bramr('amp_EQ3_coeff_imag',32)
		self.Coefficients.b4i=self.bramr('amp_EQ4_coeff_imag',32)
		self.Coefficients.b5i=self.bramr('amp_EQ5_coeff_imag',32)
		self.Coefficients.b6i=self.bramr('amp_EQ6_coeff_imag',32)
		self.Coefficients.b7i=self.bramr('amp_EQ7_coeff_imag',32)
	
	def readCoefficients(self):


		b0r=self.bramr('amp_EQ0_coeff_real',32)
		b1r=self.bramr('amp_EQ1_coeff_real',32)
		b2r=self.bramr('amp_EQ2_coeff_real',32)
		b3r=self.bramr('amp_EQ3_coeff_real',32)
		b4r=self.bramr('amp_EQ4_coeff_real',32)
		b5r=self.bramr('amp_EQ5_coeff_real',32)
		b6r=self.bramr('amp_EQ6_coeff_real',32)
		b7r=self.bramr('amp_EQ7_coeff_real',32)
		
		b0i=self.bramr('amp_EQ0_coeff_imag',32)
		b1i=self.bramr('amp_EQ1_coeff_imag',32)
		b2i=self.bramr('amp_EQ2_coeff_imag',32)
		b3i=self.bramr('amp_EQ3_coeff_imag',32)
		b4i=self.bramr('amp_EQ4_coeff_imag',32)
		b5i=self.bramr('amp_EQ5_coeff_imag',32)
		b6i=self.bramr('amp_EQ6_coeff_imag',32)
		b7i=self.bramr('amp_EQ7_coeff_imag',32)
		
		Coffs1 = numpy.zeros(64,complex)
		Coffs2 = numpy.zeros(64,complex)
		Coffs3 = numpy.zeros(64,complex)
		Coffs4 = numpy.zeros(64,complex)
		
		Coffs1[::2]=b0r + 1j*b0i
		Coffs1[1::2]=b1r + 1j*b1i
		Coffs2[::2]=b2r + 1j*b2i
		Coffs2[1::2]=b3r + 1j*b3i
		Coffs3[::2]=b4r + 1j*b4i
		Coffs3[1::2]=b5r + 1j*b5i
		Coffs4[::2]=b6r + 1j*b6i
		Coffs4[1::2]=b7r + 1j*b7i
		return [Coffs1,Coffs2,Coffs3,Coffs4]


	def rotateByVector(self,theta1,theta2,theta3,theta4):
		
		self.Coefficients.b0r=self.bramr('amp_EQ0_coeff_real',32)
		self.Coefficients.b1r=self.bramr('amp_EQ1_coeff_real',32)
		self.Coefficients.b2r=self.bramr('amp_EQ2_coeff_real',32)
		self.Coefficients.b3r=self.bramr('amp_EQ3_coeff_real',32)
		self.Coefficients.b4r=self.bramr('amp_EQ4_coeff_real',32)
		self.Coefficients.b5r=self.bramr('amp_EQ5_coeff_real',32)
		self.Coefficients.b6r=self.bramr('amp_EQ6_coeff_real',32)
		self.Coefficients.b7r=self.bramr('amp_EQ7_coeff_real',32)
		
		self.Coefficients.b0i=self.bramr('amp_EQ0_coeff_imag',32)
		self.Coefficients.b1i=self.bramr('amp_EQ1_coeff_imag',32)
		self.Coefficients.b2i=self.bramr('amp_EQ2_coeff_imag',32)
		self.Coefficients.b3i=self.bramr('amp_EQ3_coeff_imag',32)
		self.Coefficients.b4i=self.bramr('amp_EQ4_coeff_imag',32)
		self.Coefficients.b5i=self.bramr('amp_EQ5_coeff_imag',32)
		self.Coefficients.b6i=self.bramr('amp_EQ6_coeff_imag',32)
		self.Coefficients.b7i=self.bramr('amp_EQ7_coeff_imag',32)

		self.Coefficients.Coeffs1[::2]=self.Coefficients.b0r + 1j*self.Coefficients.b0i
		self.Coefficients.Coeffs1[1::2]=self.Coefficients.b1r + 1j*self.Coefficients.b1i
		self.Coefficients.Coeffs2[::2]=self.Coefficients.b2r + 1j*self.Coefficients.b2i
		self.Coefficients.Coeffs2[1::2]=self.Coefficients.b3r + 1j*self.Coefficients.b3i
		self.Coefficients.Coeffs3[::2]=self.Coefficients.b4r + 1j*self.Coefficients.b4i
		self.Coefficients.Coeffs3[1::2]=self.Coefficients.b5r + 1j*self.Coefficients.b5i
		self.Coefficients.Coeffs4[::2]=self.Coefficients.b6r + 1j*self.Coefficients.b6i
		self.Coefficients.Coeffs4[1::2]=self.Coefficients.b7r + 1j*self.Coefficients.b7i
	
		self.Coefficients.Coeffs1 = self.Coefficients.Coeffs1*numpy.exp(1j*theta1)
		self.Coefficients.Coeffs2 = self.Coefficients.Coeffs1*numpy.exp(1j*theta2)
		self.Coefficients.Coeffs3 = self.Coefficients.Coeffs1*numpy.exp(1j*theta3)
		self.Coefficients.Coeffs4 = self.Coefficients.Coeffs1*numpy.exp(1j*theta4)
		
		

		coffs1evenr=numpy.round(numpy.real(self.Coefficients.Coeffs1[::2]))
		coffs1oddr=numpy.round(numpy.real(self.Coefficients.Coeffs1[1::2]))
		coffs2evenr=numpy.round(numpy.real(self.Coefficients.Coeffs2[::2]))
		coffs2oddr=numpy.round(numpy.real(self.Coefficients.Coeffs2[1::2]))
		coffs3evenr=numpy.round(numpy.real(self.Coefficients.Coeffs3[::2]))
		coffs3oddr=numpy.round(numpy.real(self.Coefficients.Coeffs3[1::2]))
		coffs4evenr=numpy.round(numpy.real(self.Coefficients.Coeffs4[::2]))
		coffs4oddr=numpy.round(numpy.real(self.Coefficients.Coeffs4[1::2]))
		
		coffs1eveni=numpy.round(numpy.imag(self.Coefficients.Coeffs1[::2]))
		coffs1oddi=numpy.round(numpy.imag(self.Coefficients.Coeffs1[1::2]))
		coffs2eveni=numpy.round(numpy.imag(self.Coefficients.Coeffs2[::2]))
		coffs2oddi=numpy.round(numpy.imag(self.Coefficients.Coeffs2[1::2]))
		coffs3eveni=numpy.round(numpy.imag(self.Coefficients.Coeffs3[::2]))
		coffs3oddi=numpy.round(numpy.imag(self.Coefficients.Coeffs3[1::2]))
		coffs4eveni=numpy.round(numpy.imag(self.Coefficients.Coeffs4[::2]))
		coffs4oddi=numpy.round(numpy.imag(self.Coefficients.Coeffs4[1::2]))
		
		self.bramw('amp_EQ0_coeff_real', coffs1evenr, 32)
		self.bramw('amp_EQ1_coeff_real', coffs1oddr, 32)
		self.bramw('amp_EQ2_coeff_real', coffs2evenr, 32)
		self.bramw('amp_EQ3_coeff_real', coffs2oddr, 32)
		self.bramw('amp_EQ4_coeff_real', coffs3evenr, 32)
		self.bramw('amp_EQ5_coeff_real', coffs3oddr, 32)
		self.bramw('amp_EQ6_coeff_real', coffs4evenr, 32)
		self.bramw('amp_EQ7_coeff_real', coffs4oddr, 32)
		self.bramw('amp_EQ0_coeff_imag', coffs1eveni, 32)
		self.bramw('amp_EQ1_coeff_imag', coffs1oddi, 32)
		self.bramw('amp_EQ2_coeff_imag', coffs2eveni, 32)
		self.bramw('amp_EQ3_coeff_imag', coffs2oddi, 32)
		self.bramw('amp_EQ4_coeff_imag', coffs3eveni, 32)
		self.bramw('amp_EQ5_coeff_imag', coffs3oddi, 32)
		self.bramw('amp_EQ6_coeff_imag', coffs4eveni, 32)
		self.bramw('amp_EQ7_coeff_imag', coffs4oddi, 32)

		self.Coefficients.b0r=self.bramr('amp_EQ0_coeff_real',32)
		self.Coefficients.b1r=self.bramr('amp_EQ1_coeff_real',32)
		self.Coefficients.b2r=self.bramr('amp_EQ2_coeff_real',32)
		self.Coefficients.b3r=self.bramr('amp_EQ3_coeff_real',32)
		self.Coefficients.b4r=self.bramr('amp_EQ4_coeff_real',32)
		self.Coefficients.b5r=self.bramr('amp_EQ5_coeff_real',32)
		self.Coefficients.b6r=self.bramr('amp_EQ6_coeff_real',32)
		self.Coefficients.b7r=self.bramr('amp_EQ7_coeff_real',32)
		
		self.Coefficients.b0i=self.bramr('amp_EQ0_coeff_imag',32)
		self.Coefficients.b1i=self.bramr('amp_EQ1_coeff_imag',32)
		self.Coefficients.b2i=self.bramr('amp_EQ2_coeff_imag',32)
		self.Coefficients.b3i=self.bramr('amp_EQ3_coeff_imag',32)
		self.Coefficients.b4i=self.bramr('amp_EQ4_coeff_imag',32)
		self.Coefficients.b5i=self.bramr('amp_EQ5_coeff_imag',32)
		self.Coefficients.b6i=self.bramr('amp_EQ6_coeff_imag',32)
		self.Coefficients.b7i=self.bramr('amp_EQ7_coeff_imag',32)

		self.Coefficients.Coeffs1[::2]=self.Coefficients.b0r + 1j*self.Coefficients.b0i
		self.Coefficients.Coeffs1[1::2]=self.Coefficients.b1r + 1j*self.Coefficients.b1i
		self.Coefficients.Coeffs2[::2]=self.Coefficients.b2r + 1j*self.Coefficients.b2i
		self.Coefficients.Coeffs2[1::2]=self.Coefficients.b3r + 1j*self.Coefficients.b3i
		self.Coefficients.Coeffs3[::2]=self.Coefficients.b4r + 1j*self.Coefficients.b4i
		self.Coefficients.Coeffs3[1::2]=self.Coefficients.b5r + 1j*self.Coefficients.b5i
		self.Coefficients.Coeffs4[::2]=self.Coefficients.b6r + 1j*self.Coefficients.b6i
		self.Coefficients.Coeffs4[1::2]=self.Coefficients.b7r + 1j*self.Coefficients.b7i
 
	def initialiseCoefficients(self,scale):
	#this function initialises all the real coefficients to a number so you will at least see some power output
		a=scale*numpy.ones(32)	
		b=1*numpy.zeros(32)	
		self.bramw('amp_EQ0_coeff_real', a, 32)
		self.bramw('amp_EQ1_coeff_real', a, 32)
		self.bramw('amp_EQ2_coeff_real', a, 32)
		self.bramw('amp_EQ3_coeff_real', a, 32)
		self.bramw('amp_EQ4_coeff_real', a, 32)
		self.bramw('amp_EQ5_coeff_real', a, 32)
		self.bramw('amp_EQ6_coeff_real', a, 32)
		self.bramw('amp_EQ7_coeff_real', a, 32)
		self.bramw('amp_EQ0_coeff_imag', b, 32)
		self.bramw('amp_EQ1_coeff_imag', b, 32)
		self.bramw('amp_EQ2_coeff_imag', b, 32)
		self.bramw('amp_EQ3_coeff_imag', b, 32)
		self.bramw('amp_EQ4_coeff_imag', b, 32)
		self.bramw('amp_EQ5_coeff_imag', b, 32)
		self.bramw('amp_EQ6_coeff_imag', b, 32)
		self.bramw('amp_EQ7_coeff_imag', b, 32)
		self.Coefficients.b0r=self.bramr('amp_EQ0_coeff_real',32)
		self.Coefficients.b1r=self.bramr('amp_EQ1_coeff_real',32)
		self.Coefficients.b2r=self.bramr('amp_EQ2_coeff_real',32)
		self.Coefficients.b3r=self.bramr('amp_EQ3_coeff_real',32)
		self.Coefficients.b4r=self.bramr('amp_EQ4_coeff_real',32)
		self.Coefficients.b5r=self.bramr('amp_EQ5_coeff_real',32)
		self.Coefficients.b6r=self.bramr('amp_EQ6_coeff_real',32)
		self.Coefficients.b7r=self.bramr('amp_EQ7_coeff_real',32)
		
		self.Coefficients.b0i=self.bramr('amp_EQ0_coeff_imag',32)
		self.Coefficients.b1i=self.bramr('amp_EQ1_coeff_imag',32)
		self.Coefficients.b2i=self.bramr('amp_EQ2_coeff_imag',32)
		self.Coefficients.b3i=self.bramr('amp_EQ3_coeff_imag',32)
		self.Coefficients.b4i=self.bramr('amp_EQ4_coeff_imag',32)
		self.Coefficients.b5i=self.bramr('amp_EQ5_coeff_imag',32)
		self.Coefficients.b6i=self.bramr('amp_EQ6_coeff_imag',32)
		self.Coefficients.b7i=self.bramr('amp_EQ7_coeff_imag',32)

		self.Coefficients.Coeffs1[::2]=self.Coefficients.b0r + 1j*self.Coefficients.b0i
		self.Coefficients.Coeffs1[1::2]=self.Coefficients.b1r + 1j*self.Coefficients.b1i
		self.Coefficients.Coeffs2[::2]=self.Coefficients.b2r + 1j*self.Coefficients.b2i
		self.Coefficients.Coeffs2[1::2]=self.Coefficients.b3r + 1j*self.Coefficients.b3i
		self.Coefficients.Coeffs3[::2]=self.Coefficients.b4r + 1j*self.Coefficients.b4i
		self.Coefficients.Coeffs3[1::2]=self.Coefficients.b5r + 1j*self.Coefficients.b5i
		self.Coefficients.Coeffs4[::2]=self.Coefficients.b6r + 1j*self.Coefficients.b6i
		self.Coefficients.Coeffs4[1::2]=self.Coefficients.b7r + 1j*self.Coefficients.b7i


	def diodeOn(self):
	#self explanatory I hope??
	    self.fpga.write_int('gpio_set4',1)
	    self.status.noiseDiode=1
	    return 1

	def diodeOff(self):
	#self explanatory I hope??
	    self.fpga.write_int('gpio_set4',0)
	    self.status.noiseDiode=0
	    return 0

	def bramr(self,bname, samples=1024):
	#function to read a bram register
	    bramvals=self.fpga.read(bname,samples*4,0)  	    
	    string = '>'+str(samples)+'i'
	    b_0=struct.unpack(string,bramvals)
	    data=numpy.array(b_0)	
	    return data

	def bramw(self,bname, odata, samples=1024):
	    b_0=struct.pack('>'+str(samples)+'i',*odata)
	    self.fpga.write(bname,b_0)
	    return

