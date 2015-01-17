#script that runs after running the initialisation script for the ipython interface run roachPowerCalibrationIpython.py pumba pumba

import math
import time

bitstreampower = 'cbassrx_2mar_pow_2012_Mar_02_1638'
bitstreampower = bitstreampower+'.bof'
bitstreampol = 'cbassrx_6mara_pol_2012_Mar_06_1041'
bitstreampol = 'cbassrx_12feb_250pol_2012_Feb_12_1718'
bitstreampol = bitstreampol+'.bof'

katcp_port=7147
sleep_time=0.005
desiredPower=1e12






initialiseCoefficients(fpga)
time.sleep(1)

diodeOn(fpga)
time.sleep(0.5)
updateIntegration(fpga)
Channel1On=readIntegration64bit(fpga,'Subsystem1_ch1','Subsystem1_ch2')
time.sleep(0.1)
Channel2On=readIntegration64bit(fpga,'Subsystem1_ch3','Subsystem1_ch4')
time.sleep(0.1)
Channel3On=readIntegration64bit(fpga,'Subsystem1_ch5','Subsystem1_ch6')
time.sleep(0.1)
Channel4On=readIntegration64bit(fpga,'Subsystem1_ch7','Subsystem1_ch8')



diodeOff(fpga)
time.sleep(0.5)
updateIntegration(fpga)
Channel1Off=readIntegration64bit(fpga,'Subsystem1_ch1','Subsystem1_ch2')
time.sleep(0.1)
Channel2Off=readIntegration64bit(fpga,'Subsystem1_ch3','Subsystem1_ch4')
time.sleep(0.1)
Channel3Off=readIntegration64bit(fpga,'Subsystem1_ch5','Subsystem1_ch6')
time.sleep(0.1)
Channel4Off=readIntegration64bit(fpga,'Subsystem1_ch7','Subsystem1_ch8')

pylab.subplot(411)
pylab.plot(Channel1On)
pylab.plot(Channel1Off)

pylab.subplot(412)
pylab.plot(Channel2On)
pylab.plot(Channel2Off)

pylab.subplot(413)
pylab.plot(Channel3On)
pylab.plot(Channel3Off)

pylab.subplot(414)
pylab.plot(Channel4On)
pylab.plot(Channel4Off)
pylab.show()


#by now we have the subplot of the thingie
#let's work out the coefficients that we need
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

nIntegrations=78125
desiredResponse=1e14#desired power levels
Channel1Coffs=desiredResponse/Channel1On
Channel2Coffs=desiredResponse/Channel2On
Channel3Coffs=desiredResponse/Channel3On
Channel4Coffs=desiredResponse/Channel4On


#the real coefficients will be the sqrt of the coefficients calculated above
Coffs1R=numpy.sqrt(Channel1Coffs)
Coffs2R=numpy.sqrt(Channel2Coffs)
Coffs3R=numpy.sqrt(Channel3Coffs)
Coffs4R=numpy.sqrt(Channel4Coffs)

#offs1R=smoothListGaussian(Coffs1R)
#offs2R=smoothListGaussian(Coffs2R)
#offs3R=smoothListGaussian(Coffs3R)
#offs4R=smoothListGaussian(Coffs4R)


#Now we need to rearrange because we have odds and evens
Coffs1R[::2]=numpy.round(Coffs1R[::2]*b0r)
Coffs1R[1::2]=numpy.round(Coffs1R[1::2]*b1r)
Coffs2R[::2]=numpy.round(Coffs2R[::2]*b2r)
Coffs2R[1::2]=numpy.round(Coffs2R[1::2]*b3r)
Coffs3R[::2]=numpy.round(Coffs3R[::2]*b4r)
Coffs3R[1::2]=numpy.round(Coffs3R[1::2]*b5r)
Coffs4R[::2]=numpy.round(Coffs4R[::2]*b6r)
Coffs4R[1::2]=numpy.round(Coffs4R[1::2]*b7r)


#Now we write the new coefficients to the fpga
bramw(fpga,'amp_EQ0_coeff_real', Coffs1R[::2], 32)
bramw(fpga,'amp_EQ1_coeff_real', Coffs1R[1::2], 32)
bramw(fpga,'amp_EQ2_coeff_real', Coffs2R[::2], 32)
bramw(fpga,'amp_EQ3_coeff_real', Coffs2R[1::2], 32)
bramw(fpga,'amp_EQ4_coeff_real', Coffs3R[::2], 32)
bramw(fpga,'amp_EQ5_coeff_real', Coffs3R[1::2], 32)
bramw(fpga,'amp_EQ6_coeff_real', Coffs4R[::2], 32)
bramw(fpga,'amp_EQ7_coeff_real', Coffs4R[1::2], 32)

time.sleep(0.5)

diodeOn(fpga)
time.sleep(0.5)
updateIntegration(fpga)
Channel1On=readIntegration64bit(fpga,'Subsystem1_ch1','Subsystem1_ch2')
Channel2On=readIntegration64bit(fpga,'Subsystem1_ch3','Subsystem1_ch4')
Channel3On=readIntegration64bit(fpga,'Subsystem1_ch5','Subsystem1_ch6')
Channel4On=readIntegration64bit(fpga,'Subsystem1_ch7','Subsystem1_ch8')
pylab.show()



diodeOff(fpga)
time.sleep(0.5)
updateIntegration(fpga)
Channel1Off=readIntegration64bit(fpga,'Subsystem1_ch1','Subsystem1_ch2')
Channel2Off=readIntegration64bit(fpga,'Subsystem1_ch3','Subsystem1_ch4')
Channel3Off=readIntegration64bit(fpga,'Subsystem1_ch5','Subsystem1_ch6')
Channel4Off=readIntegration64bit(fpga,'Subsystem1_ch7','Subsystem1_ch8')


pylab.figure()
pylab.subplot(411)
pylab.plot(Channel1On)
pylab.plot(Channel1Off)

pylab.subplot(412)
pylab.plot(Channel2On)
pylab.plot(Channel2Off)

pylab.subplot(413)
pylab.plot(Channel3On)
pylab.plot(Channel3Off)

pylab.subplot(414)
pylab.plot(Channel4On)
pylab.plot(Channel4Off)

## by this stage we should have evened out the gain of the four channels
#Next we need to make sure that the noise diode is only in the sky channel
#let's back these up in case we need to revert

Coffs1RP=Coffs1R
Coffs2RP=Coffs2R
Coffs3RP=Coffs3R
Coffs4RP=Coffs4R
Coffs1IP=Coffs1I
Coffs2IP=Coffs2I
Coffs3IP=Coffs3I
Coffs4IP=Coffs4I

bramw(fpga,'amp_EQ0_coeff_real', Coffs1RP[::2], 32)
bramw(fpga,'amp_EQ1_coeff_real', Coffs1RP[1::2], 32)
bramw(fpga,'amp_EQ2_coeff_real', Coffs2RP[::2], 32)
bramw(fpga,'amp_EQ3_coeff_real', Coffs2RP[1::2], 32)
bramw(fpga,'amp_EQ4_coeff_real', Coffs3RP[::2], 32)
bramw(fpga,'amp_EQ5_coeff_real', Coffs3RP[1::2], 32)
bramw(fpga,'amp_EQ6_coeff_real', Coffs4RP[::2], 32)
bramw(fpga,'amp_EQ7_coeff_real', Coffs4RP[1::2], 32)

bramw(fpga,'amp_EQ0_coeff_imag', Coffs1IP[::2], 32)
bramw(fpga,'amp_EQ1_coeff_imag', Coffs1IP[1::2], 32)
bramw(fpga,'amp_EQ2_coeff_imag', Coffs2IP[::2], 32)
bramw(fpga,'amp_EQ3_coeff_imag', Coffs2IP[1::2], 32)
bramw(fpga,'amp_EQ4_coeff_imag', Coffs3IP[::2], 32)
bramw(fpga,'amp_EQ5_coeff_imag', Coffs3IP[1::2], 32)
bramw(fpga,'amp_EQ6_coeff_imag', Coffs4IP[::2], 32)
bramw(fpga,'amp_EQ7_coeff_imag', Coffs4IP[1::2], 32)


#We need to end the power script now and switch to the polarisation script



fpga.progdev(bitstreampol)

fpga.write_int('acc_len',78125)
print 'done',str(opts.acc_len)
print 'Resetting counters...',
fpga.write_int('cnt_rst',1)
fpga.write_int('cnt_rst',0)
fpga.write_int('snap_we',0)
fpga.write_int('snap_we',1)
fpga.write_int('sync_en',1)
fpga.write_int('snap_we',1)
fpga.write_int('ctrl_sw',43690) #fft size
fpga.write_int('acc_sync_delay',3) #fft size This has a different length than before!!


#write the coefficients we created in the last step
bramw(fpga,'amp_EQ0_coeff_real', Coffs1RP[::2], 32)
bramw(fpga,'amp_EQ1_coeff_real', Coffs1RP[1::2], 32)
bramw(fpga,'amp_EQ2_coeff_real', Coffs2RP[::2], 32)
bramw(fpga,'amp_EQ3_coeff_real', Coffs2RP[1::2], 32)
bramw(fpga,'amp_EQ4_coeff_real', Coffs3RP[::2], 32)
bramw(fpga,'amp_EQ5_coeff_real', Coffs3RP[1::2], 32)
bramw(fpga,'amp_EQ6_coeff_real', Coffs4RP[::2], 32)
bramw(fpga,'amp_EQ7_coeff_real', Coffs4RP[1::2], 32)

bramw(fpga,'amp_EQ0_coeff_imag', Coffs1IP[::2], 32)
bramw(fpga,'amp_EQ1_coeff_imag', Coffs1IP[1::2], 32)
bramw(fpga,'amp_EQ2_coeff_imag', Coffs2IP[::2], 32)
bramw(fpga,'amp_EQ3_coeff_imag', Coffs2IP[1::2], 32)
bramw(fpga,'amp_EQ4_coeff_imag', Coffs3IP[::2], 32)
bramw(fpga,'amp_EQ5_coeff_imag', Coffs3IP[1::2], 32)
bramw(fpga,'amp_EQ6_coeff_imag', Coffs4IP[::2], 32)
bramw(fpga,'amp_EQ7_coeff_imag', Coffs4IP[1::2], 32)


diodeOn(fpga)
time.sleep(0.1)
updateIntegration(fpga)

RROn=readIntegration64bit(fpga,'Subsystem1_ch1','Subsystem1_ch2')
LLOn=readIntegration64bit(fpga,'Subsystem1_ch3','Subsystem1_ch4')
QOn=readIntegration64bit(fpga,'Subsystem1_ch5','Subsystem1_ch6')
UOn=readIntegration64bit(fpga,'Subsystem1_ch7','Subsystem1_ch8')
RloadOn=readIntegration64bit(fpga,'Subsystem1_ch9','Subsystem1_ch10')
LloadOn=readIntegration64bit(fpga,'Subsystem1_ch11','Subsystem1_ch12')

diodeOff(fpga)
time.sleep(0.1)
updateIntegration(fpga)
RROff=readIntegration64bit(fpga,'Subsystem1_ch1','Subsystem1_ch2')
LLOff=readIntegration64bit(fpga,'Subsystem1_ch3','Subsystem1_ch4')
QOff=readIntegration64bit(fpga,'Subsystem1_ch5','Subsystem1_ch6')
UOff=readIntegration64bit(fpga,'Subsystem1_ch7','Subsystem1_ch8')
RloadOff=readIntegration64bit(fpga,'Subsystem1_ch9','Subsystem1_ch10')
LloadOff=readIntegration64bit(fpga,'Subsystem1_ch11','Subsystem1_ch12')


pylab.figure()
pylab.subplot(411)
pylab.plot(RROn)
pylab.plot(RROff)

pylab.subplot(412)
pylab.plot(RloadOn)
pylab.plot(RloadOff)

pylab.subplot(413)
pylab.plot(LLOn)
pylab.plot(LLOff)


pylab.subplot(414)
pylab.plot(LloadOn)
pylab.plot(LloadOff)





#get the coefficients
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

#instatiate the coefficients

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

#calculate the amount of power in the sky and load channels as an indicator of rotation required
dL=LLOn-LLOff
dR=RROn-RROff
dLRef=LloadOn-LloadOff
dRRef=RloadOn-RloadOff
window_size=5 ##size of the moving average
window= numpy.ones(int(window_size))/float(window_size) # moving average window


dLsmooth= numpy.convolve(dL,window,'same')
dRsmooth= numpy.convolve(dR,window,'same')
dLRefsmooth= numpy.convolve(dLRef,window,'same')
dRRefsmooth= numpy.convolve(dRRef,window,'same')


t1=dLsmooth/dLRefsmooth-1.
t2=dLsmooth/dLRefsmooth+1.
t3=dRsmooth/dRRefsmooth-1.
t4=dRsmooth/dRRefsmooth+1.





#calculate the angle to rotate by
thetaL=numpy.arccos(t1/t2)
thetaR=numpy.arccos(t3/t4)
thetaL=numpy.nan_to_num(thetaL)
thetaR=numpy.nan_to_num(thetaR)
thetaLsmooth= numpy.convolve(thetaL,window,'same')
thetaRsmooth= numpy.convolve(thetaR,window,'same')

#thetaLsmooth=thetaLsmooth-numpy.mean(thetaLsmooth)
#thetaRsmooth=thetaRsmooth-numpy.mean(thetaRsmooth)

Coffs2Complex=Coffs2Complex*numpy.exp(1j*thetaRsmooth)
Coffs4Complex=Coffs4Complex*numpy.exp(1j*thetaLsmooth)
Coffs2R=numpy.round(numpy.real(Coffs2Complex))
Coffs2I=numpy.round(numpy.imag(Coffs2Complex))
Coffs4R=numpy.round(numpy.real(Coffs4Complex))
Coffs4I=numpy.round(numpy.imag(Coffs4Complex))


pylab.figure()
pylab.subplot(311)
pylab.plot(thetaRsmooth)
pylab.plot(thetaLsmooth)
pylab.legend(('RR','LL'))
pylab.title('Rotation Angle')

pylab.subplot(312)
pylab.plot(Coffs2R)
pylab.plot(Coffs2I)
pylab.title('RR coefficients')
pylab.legend(('Real','Imag'))

pylab.subplot(313)
pylab.plot(Coffs4R)
pylab.plot(Coffs4I)
pylab.title('LL coefficients')
pylab.legend(('Real','Imag'))




#write the new coefficients Vcreated in the last step
bramw(fpga,'amp_EQ0_coeff_real', Coffs1R[::2], 32)
bramw(fpga,'amp_EQ1_coeff_real', Coffs1R[1::2], 32)
bramw(fpga,'amp_EQ2_coeff_real', Coffs2R[::2], 32)
bramw(fpga,'amp_EQ3_coeff_real', Coffs2R[1::2], 32)
bramw(fpga,'amp_EQ4_coeff_real', Coffs3R[::2], 32)
bramw(fpga,'amp_EQ5_coeff_real', Coffs3R[1::2], 32)
bramw(fpga,'amp_EQ6_coeff_real', Coffs4R[::2], 32)
bramw(fpga,'amp_EQ7_coeff_real', Coffs4R[1::2], 32)
bramw(fpga,'amp_EQ0_coeff_imag', Coffs1I[::2], 32)
bramw(fpga,'amp_EQ1_coeff_imag', Coffs1I[1::2], 32)
bramw(fpga,'amp_EQ2_coeff_imag', Coffs2I[::2], 32)
bramw(fpga,'amp_EQ3_coeff_imag', Coffs2I[1::2], 32)
bramw(fpga,'amp_EQ4_coeff_imag', Coffs3I[::2], 32)
bramw(fpga,'amp_EQ5_coeff_imag', Coffs3I[1::2], 32)
bramw(fpga,'amp_EQ6_coeff_imag', Coffs4I[::2], 32)
bramw(fpga,'amp_EQ7_coeff_imag', Coffs4I[1::2], 32)




diodeOn(fpga)
time.sleep(0.1)
updateIntegration(fpga)

RROn=readIntegration64bit(fpga,'Subsystem1_ch1','Subsystem1_ch2')
LLOn=readIntegration64bit(fpga,'Subsystem1_ch3','Subsystem1_ch4')
QOn=readIntegration64bit(fpga,'Subsystem1_ch5','Subsystem1_ch6')
UOn=readIntegration64bit(fpga,'Subsystem1_ch7','Subsystem1_ch8')
RloadOn=readIntegration64bit(fpga,'Subsystem1_ch9','Subsystem1_ch10')
LloadOn=readIntegration64bit(fpga,'Subsystem1_ch11','Subsystem1_ch12')

diodeOff(fpga)
time.sleep(0.1)
updateIntegration(fpga)
RROff=readIntegration64bit(fpga,'Subsystem1_ch1','Subsystem1_ch2')
LLOff=readIntegration64bit(fpga,'Subsystem1_ch3','Subsystem1_ch4')
QOff=readIntegration64bit(fpga,'Subsystem1_ch5','Subsystem1_ch6')
UOff=readIntegration64bit(fpga,'Subsystem1_ch7','Subsystem1_ch8')
RloadOff=readIntegration64bit(fpga,'Subsystem1_ch9','Subsystem1_ch10')
LloadOff=readIntegration64bit(fpga,'Subsystem1_ch11','Subsystem1_ch12')


pylab.figure()
pylab.subplot(411)
pylab.plot(RROn)
pylab.plot(RROff)
pylab.title('RR')

pylab.subplot(412)
pylab.plot(RloadOn)
pylab.plot(RloadOff)
pylab.title('RLoad')

pylab.subplot(413)
pylab.plot(LLOn)
pylab.plot(LLOff)
pylab.title('LL')


pylab.subplot(414)
pylab.plot(LloadOn)
pylab.plot(LloadOff)
pylab.title('LLoad')



###############REPEAT THE NEXT PART UNTIL THE LOAD IS SPEARATED#############
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


diodeOn(fpga)
time.sleep(0.1)
updateIntegration(fpga)

RROn=readIntegration64bit(fpga,'Subsystem1_ch1','Subsystem1_ch2')
LLOn=readIntegration64bit(fpga,'Subsystem1_ch3','Subsystem1_ch4')
QOn=readIntegration64bit(fpga,'Subsystem1_ch5','Subsystem1_ch6')
UOn=readIntegration64bit(fpga,'Subsystem1_ch7','Subsystem1_ch8')
RloadOn=readIntegration64bit(fpga,'Subsystem1_ch9','Subsystem1_ch10')
LloadOn=readIntegration64bit(fpga,'Subsystem1_ch11','Subsystem1_ch12')

diodeOff(fpga)
time.sleep(0.1)
updateIntegration(fpga)
RROff=readIntegration64bit(fpga,'Subsystem1_ch1','Subsystem1_ch2')
LLOff=readIntegration64bit(fpga,'Subsystem1_ch3','Subsystem1_ch4')
QOff=readIntegration64bit(fpga,'Subsystem1_ch5','Subsystem1_ch6')
UOff=readIntegration64bit(fpga,'Subsystem1_ch7','Subsystem1_ch8')
RloadOff=readIntegration64bit(fpga,'Subsystem1_ch9','Subsystem1_ch10')
LloadOff=readIntegration64bit(fpga,'Subsystem1_ch11','Subsystem1_ch12')

#calculate the amount of power in the sky and load channels as an indicator of rotation required
dL=LLOn-LLOff
dR=RROn-RROff
dLRef=LloadOn-LloadOff
dRRef=RloadOn-RloadOff
window_size=5 ##size of the moving average
window= numpy.ones(int(window_size))/float(window_size) # moving average window


#dLsmooth= numpy.convolve(dL,window,'same')
#dRsmooth= numpy.convolve(dR,window,'same')
#dLRefsmooth= numpy.convolve(dLRef,window,'same')
#dRRefsmooth= numpy.convolve(dRRef,window,'same')
dLsmooth= dL
dRsmooth= dR
dLRefsmooth= dLRef
dRRefsmooth= dRRef



t1=dLsmooth/dLRefsmooth-1.
t2=dLsmooth/dLRefsmooth+1.
t3=dRsmooth/dRRefsmooth-1.
t4=dRsmooth/dRRefsmooth+1.





#calculate the angle to rotate by
thetaL=numpy.arccos(t1/t2)
thetaR=numpy.arccos(t3/t4)
thetaL=numpy.nan_to_num(thetaL)
thetaR=numpy.nan_to_num(thetaR)
#thetaLsmooth= numpy.convolve(thetaL,window,'same')
#thetaRsmooth= numpy.convolve(thetaR,window,'same')

thetaLsmooth=thetaL
thetaRsmooth=thetaR

Coffs2Complex=Coffs2Complex*numpy.exp(1j*thetaRsmooth)
Coffs4Complex=Coffs4Complex*numpy.exp(1j*thetaLsmooth)
Coffs2R=numpy.round(numpy.real(Coffs2Complex))
Coffs2I=numpy.round(numpy.imag(Coffs2Complex))
Coffs4R=numpy.round(numpy.real(Coffs4Complex))
Coffs4I=numpy.round(numpy.imag(Coffs4Complex))


pylab.figure()
pylab.subplot(311)
pylab.plot(thetaRsmooth)
pylab.plot(thetaLsmooth)
pylab.legend(('RR','LL'))
pylab.title('Rotation Angle')

pylab.subplot(312)
pylab.plot(Coffs2R)
pylab.plot(Coffs2I)
pylab.title('RR coefficients')
pylab.legend(('Real','Imag'))

pylab.subplot(313)
pylab.plot(Coffs4R)
pylab.plot(Coffs4I)
pylab.title('LL coefficients')
pylab.legend(('Real','Imag'))




#write the new coefficients Vcreated in the last step
bramw(fpga,'amp_EQ0_coeff_real', Coffs1R[::2], 32)
bramw(fpga,'amp_EQ1_coeff_real', Coffs1R[1::2], 32)
bramw(fpga,'amp_EQ2_coeff_real', Coffs2R[::2], 32)
bramw(fpga,'amp_EQ3_coeff_real', Coffs2R[1::2], 32)
bramw(fpga,'amp_EQ4_coeff_real', Coffs3R[::2], 32)
bramw(fpga,'amp_EQ5_coeff_real', Coffs3R[1::2], 32)
bramw(fpga,'amp_EQ6_coeff_real', Coffs4R[::2], 32)
bramw(fpga,'amp_EQ7_coeff_real', Coffs4R[1::2], 32)
bramw(fpga,'amp_EQ0_coeff_imag', Coffs1I[::2], 32)
bramw(fpga,'amp_EQ1_coeff_imag', Coffs1I[1::2], 32)
bramw(fpga,'amp_EQ2_coeff_imag', Coffs2I[::2], 32)
bramw(fpga,'amp_EQ3_coeff_imag', Coffs2I[1::2], 32)
bramw(fpga,'amp_EQ4_coeff_imag', Coffs3I[::2], 32)
bramw(fpga,'amp_EQ5_coeff_imag', Coffs3I[1::2], 32)
bramw(fpga,'amp_EQ6_coeff_imag', Coffs4I[::2], 32)
bramw(fpga,'amp_EQ7_coeff_imag', Coffs4I[1::2], 32)


diodeOn(fpga)
time.sleep(0.1)
updateIntegration(fpga)

RROn=readIntegration64bit(fpga,'Subsystem1_ch1','Subsystem1_ch2')
LLOn=readIntegration64bit(fpga,'Subsystem1_ch3','Subsystem1_ch4')
QOn=readIntegration64bit(fpga,'Subsystem1_ch5','Subsystem1_ch6')
UOn=readIntegration64bit(fpga,'Subsystem1_ch7','Subsystem1_ch8')
RloadOn=readIntegration64bit(fpga,'Subsystem1_ch9','Subsystem1_ch10')
LloadOn=readIntegration64bit(fpga,'Subsystem1_ch11','Subsystem1_ch12')

diodeOff(fpga)
time.sleep(0.1)
updateIntegration(fpga)
RROff=readIntegration64bit(fpga,'Subsystem1_ch1','Subsystem1_ch2')
LLOff=readIntegration64bit(fpga,'Subsystem1_ch3','Subsystem1_ch4')
QOff=readIntegration64bit(fpga,'Subsystem1_ch5','Subsystem1_ch6')
UOff=readIntegration64bit(fpga,'Subsystem1_ch7','Subsystem1_ch8')
RloadOff=readIntegration64bit(fpga,'Subsystem1_ch9','Subsystem1_ch10')
LloadOff=readIntegration64bit(fpga,'Subsystem1_ch11','Subsystem1_ch12')


pylab.figure()
pylab.subplot(411)
pylab.plot(RROn)
pylab.plot(RROff)
pylab.title('RR')

pylab.subplot(412)
pylab.plot(RloadOn)
pylab.plot(RloadOff)
pylab.title('RLoad')

pylab.subplot(413)
pylab.plot(LLOn)
pylab.plot(LLOff)
pylab.title('LL')


pylab.subplot(414)
pylab.plot(LloadOn)
pylab.plot(LloadOff)
pylab.title('LLoad')
###############END REPEATD#############




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


Coffs2Complex=Coffs2Complex*numpy.exp(1j*thetaRsmooth)
Coffs4Complex=Coffs4Complex*numpy.exp(1j*thetaLsmooth)
Coffs2R=numpy.round(numpy.real(Coffs2Complex))
Coffs2I=numpy.round(numpy.imag(Coffs2Complex))
Coffs4R=numpy.round(numpy.real(Coffs4Complex))
Coffs4I=numpy.round(numpy.imag(Coffs4Complex))



#write the new coefficients Vcreated in the last step
bramw(fpga,'amp_EQ0_coeff_real', Coffs1R[::2], 32)
bramw(fpga,'amp_EQ1_coeff_real', Coffs1R[1::2], 32)
bramw(fpga,'amp_EQ2_coeff_real', Coffs2R[::2], 32)
bramw(fpga,'amp_EQ3_coeff_real', Coffs2R[1::2], 32)
bramw(fpga,'amp_EQ4_coeff_real', Coffs3R[::2], 32)
bramw(fpga,'amp_EQ5_coeff_real', Coffs3R[1::2], 32)
bramw(fpga,'amp_EQ6_coeff_real', Coffs4R[::2], 32)
bramw(fpga,'amp_EQ7_coeff_real', Coffs4R[1::2], 32)
bramw(fpga,'amp_EQ0_coeff_imag', Coffs1I[::2], 32)
bramw(fpga,'amp_EQ1_coeff_imag', Coffs1I[1::2], 32)
bramw(fpga,'amp_EQ2_coeff_imag', Coffs2I[::2], 32)
bramw(fpga,'amp_EQ3_coeff_imag', Coffs2I[1::2], 32)
bramw(fpga,'amp_EQ4_coeff_imag', Coffs3I[::2], 32)
bramw(fpga,'amp_EQ5_coeff_imag', Coffs3I[1::2], 32)
bramw(fpga,'amp_EQ6_coeff_imag', Coffs4I[::2], 32)
bramw(fpga,'amp_EQ7_coeff_imag', Coffs4I[1::2], 32)


diodeOn(fpga)
time.sleep(0.1)
updateIntegration(fpga)

RROn=readIntegration64bit(fpga,'Subsystem1_ch1','Subsystem1_ch2')
LLOn=readIntegration64bit(fpga,'Subsystem1_ch3','Subsystem1_ch4')
QOn=readIntegration64bit(fpga,'Subsystem1_ch5','Subsystem1_ch6')
UOn=readIntegration64bit(fpga,'Subsystem1_ch7','Subsystem1_ch8')
RloadOn=readIntegration64bit(fpga,'Subsystem1_ch9','Subsystem1_ch10')
LloadOn=readIntegration64bit(fpga,'Subsystem1_ch11','Subsystem1_ch12')

diodeOff(fpga)
time.sleep(0.1)
updateIntegration(fpga)
RROff=readIntegration64bit(fpga,'Subsystem1_ch1','Subsystem1_ch2')
LLOff=readIntegration64bit(fpga,'Subsystem1_ch3','Subsystem1_ch4')
QOff=readIntegration64bit(fpga,'Subsystem1_ch5','Subsystem1_ch6')
UOff=readIntegration64bit(fpga,'Subsystem1_ch7','Subsystem1_ch8')
RloadOff=readIntegration64bit(fpga,'Subsystem1_ch9','Subsystem1_ch10')
LloadOff=readIntegration64bit(fpga,'Subsystem1_ch11','Subsystem1_ch12')


pylab.figure()
pylab.subplot(411)
pylab.plot(RROn)
pylab.plot(RROff)
pylab.title('RR')

pylab.subplot(412)
pylab.plot(RloadOn)
pylab.plot(RloadOff)
pylab.title('RLoad')

pylab.subplot(413)
pylab.plot(LLOn)
pylab.plot(LLOff)
pylab.title('LL')


pylab.subplot(414)
pylab.plot(LloadOn)
pylab.plot(LloadOff)
pylab.title('LLoad')





