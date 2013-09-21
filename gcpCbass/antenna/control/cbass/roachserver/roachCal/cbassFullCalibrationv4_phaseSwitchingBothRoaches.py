#this coded performs the full calibration of the C-BASS system using low level ipython interface

import copy,pylab,time,numpy,cbass,sys,os,struct# first import the interaction module

#roachName = sys.argv[1]
#roachName = 'pumba'
#roachName = 'timon'

#os.mkdir(roachName)

Roach1=cbass.roach()
Roach2=cbass.roach()
ND=cbass.roach()
nIntegrations=781250
sleepTime=(nIntegrations/78125)*0.03
desiredResponse = 1e13


#Roach.instantiateRoach(roachName,'cbassrx_2mar_pow_2012_Mar_02_1638.bof',7147,10,nIntegrations)
#ND.instantiateRoach('pumba','cbassrx_2mar_pow_2012_Mar_02_1638.bof',7147,10,nIntegrations) ##we always use pumba for the noise diode!!

Roach1.instantiateRoach('pumba','cbassrx_2may_pow_2012_May_02_1555.bof',7147,10,nIntegrations)
Roach2.instantiateRoach('timon','cbassrx_2may_pow_2012_May_02_1555.bof',7147,10,nIntegrations)
ND.instantiateRoach('pumba','cbassrx_2may_pow_2012_May_02_1555.bof',7147,10,nIntegrations) ##we always use pumba for the noise diode!!


#initialise the class coefficients and those stored on the roach
ND.initialiseCoefficients(1000)
Roach1.initialiseCoefficients(1000)
Roach2.initialiseCoefficients(1000)


print 'FPGA? ',Roach1.fpga.is_connected()
print 'FPGA? ',Roach2.fpga.is_connected()
print 'ND? ',ND.fpga.is_connected()




#Pumba controls the noise diode!

theta=numpy.ones(64)
#rotate the channels by the following vector
#umba.rotateByVector(theta,theta,theta,theta)
Roach1.fpga.write_int('phaseDelay2',32)
Roach1.fpga.write_int('demodPhaseSwitch1',0)
#Roach1.fpga.write_int('Demod_transL',10)
Roach1.fpga.write_int('Demod1_transL',10)
Roach1.fpga.write_int('Demod2_transL',10)
Roach1.fpga.write_int('Demod3_transL',10)
Roach1.fpga.write_int('Demod4_transL',10)
#Roach1.fpga.write_int('phaseFrequency',1280) #24.26kHz
#Roach1.fpga.write_int('enablePhaseSwitch',1) 

Roach2.fpga.write_int('phaseDelay2',32)
Roach2.fpga.write_int('demodPhaseSwitch1',0)
#Roach2.fpga.write_int('Demod_transL',10)
Roach2.fpga.write_int('Demod1_transL',10)
Roach2.fpga.write_int('Demod2_transL',10)
Roach2.fpga.write_int('Demod3_transL',10)
Roach2.fpga.write_int('Demod4_transL',10)
#Roach2.fpga.write_int('phaseFrequency',1280) #24.26kHz
#Roach2.fpga.write_int('enablePhaseSwitch',1) 


for i in range(1):
    ND.diodeOn()
    time.sleep(0.5)
    fpga1Poweron = Roach1.updatePower()
    fpga2Poweron = Roach2.updatePower()
    
    #Pumba.instantiateRoach('pumba','cbassrx_2mar_pow_2012_Mar_02_1638.bof',7147,10,781250)
    #Timon.instantiateRoach('timon','cbassrx_2mar_pow_2012_Mar_02_1638.bof',7147,10,781250)
    
    ND.diodeOff()
    time.sleep(sleepTime)
    fpga1Poweroff = Roach1.updatePower()
    fpga2Poweroff = Roach2.updatePower()
    
    
    
    
    #First do pumba# here we calculate the coefficiensts needed to flatten the noise diode off
    #power spectrum
    Channel1CoffsR1=desiredResponse/fpga1Poweroff[0]
    Channel2CoffsR1=desiredResponse/fpga1Poweroff[1]
    Channel3CoffsR1=desiredResponse/fpga1Poweroff[2]
    Channel4CoffsR1=desiredResponse/fpga1Poweroff[3]
    
    Channel1CoffsR2=desiredResponse/fpga2Poweroff[0]
    Channel2CoffsR2=desiredResponse/fpga2Poweroff[1]
    Channel3CoffsR2=desiredResponse/fpga2Poweroff[2]
    Channel4CoffsR2=desiredResponse/fpga2Poweroff[3]
    
    
    #the real coefficients will be the sqrt of the coefficients calculated above
    [Coffs1R1,Coffs2R1,Coffs3R1,Coffs4R1] = Roach1.readCoefficients()
    Coffs1R1=(numpy.sqrt(Channel1CoffsR1)+1j*0) * Coffs1R1
    Coffs2R1=(numpy.sqrt(Channel2CoffsR1)+1j*0) * Coffs2R1
    Coffs3R1=(numpy.sqrt(Channel3CoffsR1)+1j*0) * Coffs3R1
    Coffs4R1=(numpy.sqrt(Channel4CoffsR1)+1j*0) * Coffs4R1
    Roach1.writeCoefficients(Coffs1R1,Coffs2R1,Coffs3R1,Coffs4R1)
    
    [Coffs1R2,Coffs2R2,Coffs3R2,Coffs4R2] = Roach2.readCoefficients()
    Coffs1R2=(numpy.sqrt(Channel1CoffsR2)+1j*0) * Coffs1R2
    Coffs2R2=(numpy.sqrt(Channel2CoffsR2)+1j*0) * Coffs2R2
    Coffs3R2=(numpy.sqrt(Channel3CoffsR2)+1j*0) * Coffs3R2
    Coffs4R2=(numpy.sqrt(Channel4CoffsR2)+1j*0) * Coffs4R2
    Roach2.writeCoefficients(Coffs1R2,Coffs2R2,Coffs3R2,Coffs4R2)


ND.diodeOn()
time.sleep(sleepTime)
fpga1Poweron = Roach1.updatePower()
fpga2Poweron = Roach2.updatePower()

ND.diodeOff()
time.sleep(sleepTime)
fpga1Poweroff = Roach1.updatePower()
fpga2Poweroff = Roach2.updatePower()


freq=numpy.arange(4.5,5,0.5/64)
ylimSmall=0.98*desiredResponse
ylimBig=1.03*desiredResponse
pylab.figure()
pylab.subplots_adjust(hspace=0.5)
pylab.subplot(411)
pylab.plot(freq,fpga1Poweron[0])
pylab.plot(freq,fpga1Poweroff[0])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel1 Pumba')
pylab.subplot(412)
pylab.plot(freq,fpga1Poweron[1])
pylab.plot(freq,fpga1Poweroff[1])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel2 Pumba')
pylab.subplot(413)
pylab.plot(freq,fpga1Poweron[2])
pylab.plot(freq,fpga1Poweroff[2])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel3 Pumba')
pylab.subplot(414)
pylab.plot(freq,fpga1Poweron[3])
pylab.plot(freq,fpga1Poweroff[3])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel4 Pumba')
pylab.savefig('pumba'+'/PowerPumba.png')
pylab.show()


freq=numpy.arange(4.5,5,0.5/64)
ylimSmall=10*numpy.log10(desiredResponse)-0.1
ylimBig=10*numpy.log10(desiredResponse)+0.2
pylab.figure()
pylab.subplots_adjust(hspace=0.5)
pylab.subplot(411)
pylab.plot(freq,10*numpy.log10(fpga1Poweron[0]))
pylab.plot(freq,10*numpy.log10(fpga1Poweroff[0]))
pylab.grid()
pylab.ylabel('dB')
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel1 Pumba')
pylab.subplot(412)
pylab.plot(freq,10*numpy.log10(fpga1Poweron[1]))
pylab.plot(freq,10*numpy.log10(fpga1Poweroff[1]))
pylab.grid()
pylab.ylabel('dB')
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel2 Pumba')
pylab.subplot(413)
pylab.plot(freq,10*numpy.log10(fpga1Poweron[2]))
pylab.plot(freq,10*numpy.log10(fpga1Poweroff[2]))
pylab.grid()
pylab.ylabel('dB')
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel3 Pumba')
pylab.subplot(414)
pylab.plot(freq,10*numpy.log10(fpga1Poweron[3]))
pylab.plot(freq,10*numpy.log10(fpga1Poweroff[3]))
pylab.grid()
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel4 Timon')
pylab.savefig('pumba'+'/PowerPumba.png')
pylab.xlabel('Frequency [GHz]')
pylab.ylabel('dB')
pylab.show()


freq=numpy.arange(5.5,5,-0.5/64)
ylimSmall=10*numpy.log10(desiredResponse)-0.1
ylimBig=10*numpy.log10(desiredResponse)+0.2
pylab.figure()
pylab.subplots_adjust(hspace=0.5)
pylab.subplot(411)
pylab.plot(freq,10*numpy.log10(fpga2Poweron[0]))
pylab.plot(freq,10*numpy.log10(fpga2Poweroff[0]))
pylab.grid()
pylab.ylabel('dB')
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel1 Timon')
pylab.subplot(412)
pylab.plot(freq,10*numpy.log10(fpga2Poweron[1]))
pylab.plot(freq,10*numpy.log10(fpga2Poweroff[1]))
pylab.grid()
pylab.ylabel('dB')
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel2 Timon')
pylab.subplot(413)
pylab.plot(freq,10*numpy.log10(fpga2Poweron[2]))
pylab.plot(freq,10*numpy.log10(fpga2Poweroff[2]))
pylab.grid()
pylab.ylabel('dB')
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel3 Timon')
pylab.subplot(414)
pylab.plot(freq,10*numpy.log10(fpga2Poweron[3]))
pylab.plot(freq,10*numpy.log10(fpga2Poweroff[3]))
pylab.grid()
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel4 Timon')
pylab.savefig('timon'+'/PowerTimon.png')
pylab.xlabel('Frequency [GHz]')
pylab.ylabel('dB')
pylab.show()

#and store the coefficients for later
[Coffs1PowerR1,Coffs2PowerR1,Coffs3PowerR1,Coffs4PowerR1] = Roach1.readCoefficients()
[Coffs1PowerR2,Coffs2PowerR2,Coffs3PowerR2,Coffs4PowerR2] = Roach2.readCoefficients()

Roach1.saveCoefficientstoDisk('pumba')
Roach2.saveCoefficientstoDisk('timon')

#Roach.instantiateRoach(roachName,'cbassrx_27nov_pol_2012_Nov_27_1431.bof',7147,4,nIntegrations)
#ND.instantiateRoach('pumba','cbassrx_27nov_pol_2012_Nov_27_1431.bof',7147,4,nIntegrations)

Roach1.instantiateRoach('pumba','rx_10dec_stat_2013_Jan_11_1059.bof',7147,4,nIntegrations)
Roach2.instantiateRoach('timon','rx_10dec_stat_2013_Jan_11_1059.bof',7147,4,nIntegrations)
ND.instantiateRoach('pumba','rx_10dec_stat_2013_Jan_11_1059.bof',7147,4,nIntegrations)

Roach1.writeCoefficients(Coffs1PowerR1,Coffs2PowerR1,Coffs3PowerR1,Coffs4PowerR1)
Roach2.writeCoefficients(Coffs1PowerR2,Coffs2PowerR2,Coffs3PowerR2,Coffs4PowerR2)


print 'Roach1 ',Roach1.fpga.is_connected()
print 'Roach2 ',Roach2.fpga.is_connected()
print 'ND ',ND.fpga.is_connected()

Roach1.fpga.write_int('phaseDelay2',32)
Roach1.fpga.write_int('demodPhaseSwitch1',1)
Roach1.fpga.write_int('Demod_transL',10)
Roach1.fpga.write_int('Demod1_transL',10)
Roach1.fpga.write_int('Demod2_transL',10)
Roach1.fpga.write_int('Demod3_transL',10)
#Roach1.fpga.write_int('phaseFrequency',1280) #24.26kHz
#Roach1.fpga.write_int('enablePhaseSwitch',1) 
Roach1.fpga.write_int('gpioMode',0) 

#Roach1.fpga.write_int('enablePhaseSwitch',0)  ##this turns off the phase switching
Roach1.fpga.write_int('gpioMode',0) 
Roach1.fpga.write_int('demodPhaseSwitch1',1) ##this turns off the demodulation

Roach2.fpga.write_int('phaseDelay2',32)
Roach2.fpga.write_int('demodPhaseSwitch1',1)
Roach2.fpga.write_int('Demod_transL',10)
Roach2.fpga.write_int('Demod1_transL',10)
Roach2.fpga.write_int('Demod2_transL',10)
Roach2.fpga.write_int('Demod3_transL',10)
#Roach2.fpga.write_int('phaseFrequency',1280) #24.26kHz
#Roach2.fpga.write_int('enablePhaseSwitch',1) 
Roach2.fpga.write_int('gpioMode',0) 

#Roach2.fpga.write_int('enablePhaseSwitch',0)  ##this turns off the phase switching
Roach2.fpga.write_int('gpioMode',0) 
Roach2.fpga.write_int('demodPhaseSwitch1',1) ##this turns off the demodulation

#Pumba controls the noise diode!
ND.diodeOn()


time.sleep(sleepTime)
roach1Polon=Roach1.updatePolarisation()
roach2Polon=Roach2.updatePolarisation()

ND.diodeOff()
time.sleep(sleepTime)

roach1Poloff = Roach1.updatePolarisation()
roach2Poloff = Roach2.updatePolarisation()

ylimSmall=10*numpy.log10(desiredResponse)-0.1
ylimBig=10*numpy.log10(desiredResponse)+0.2

pylab.figure()
pylab.subplots_adjust(hspace=1)
pylab.subplot(6,1,1)
pylab.plot(10*numpy.log10(roach1Polon[0]))
pylab.plot(10*numpy.log10(roach1Poloff[0]))
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' RR Pumba')
pylab.subplot(6,1,2)
pylab.plot(10*numpy.log10(roach1Polon[1]))
pylab.plot(10*numpy.log10(roach1Poloff[1]))
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Rload Pumba')
pylab.subplot(6,1,3)
pylab.plot(10*numpy.log10(roach1Polon[2]))
pylab.plot(10*numpy.log10(roach1Poloff[2]))
pylab.ylim([ylimSmall,ylimBig])
#pylab.title(' LL Pumba')
pylab.subplot(6,1,4)
pylab.plot(10*numpy.log10(roach1Polon[3]))
pylab.plot(10*numpy.log10(roach1Poloff[3]))
pylab.ylim([ylimSmall,ylimBig])
#pylab.title(' Lload Pumba')
pylab.subplot(6,1,5)
pylab.plot(roach1Polon[4])
pylab.plot(roach1Poloff[4])
pylab.ylim([ypollimSmall,ypollimBig])
#pylab.title(' Q Pumba')
pylab.subplot(6,1,6)
pylab.plot(roach1Polon[5])
pylab.plot(roach1Poloff[5])
pylab.ylim([ypollimSmall,ypollimBig])
pylab.title(' U Pumba')

ylimSmall=0.45*desiredResponse
ylimBig=0.6*desiredResponse
ypollimSmall = -1e12
ypollimBig = +1e12
pylab.figure()
pylab.subplots_adjust(hspace=1)
pylab.subplot(6,1,1)
pylab.plot(roach2Polon[0])
pylab.plot(roach2Poloff[0])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' RR Timon')
pylab.subplot(6,1,2)
pylab.plot(roach2Polon[1])
pylab.plot(roach2Poloff[1])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Rload Timon')
pylab.subplot(6,1,3)
pylab.plot(roach2Polon[2])
pylab.plot(roach2Poloff[2])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' LL Timon')
pylab.subplot(6,1,4)
pylab.plot(roach2Polon[3])
pylab.plot(roach2Poloff[3])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Lload Timon')
pylab.subplot(6,1,5)
pylab.plot(roach2Polon[4])
pylab.plot(roach2Poloff[4])
pylab.ylim([ypollimSmall,ypollimBig])
pylab.title(' Q Timon')
pylab.subplot(6,1,6)
pylab.plot(roach2Polon[5])
pylab.plot(roach2Poloff[5])
pylab.ylim([ypollimSmall,ypollimBig])
pylab.title(' U Timon')


##rotate the vectors using the roach coefficients
j=0
i=0;
totalAngle=0
dRef1RR=numpy.zeros((64,256))
dRef1LL=numpy.zeros((64,256))
dRef2RR=numpy.zeros((64,256))
dRef2LL=numpy.zeros((64,256))
angle=numpy.zeros(256)
window_size = 3
window= numpy.ones(int(window_size))/float(window_size) # moving average window
[Coffs1startR1,Coffs2startR1,Coffs3startR1,Coffs4startR1] = Roach1.readCoefficients()
[Coffs1startR2,Coffs2startR2,Coffs3startR2,Coffs4startR2] = Roach2.readCoefficients()
totalAngle=0
angleSize=0.1
i=0
while totalAngle<6.3:
    j=angleSize
    angle[i]=totalAngle
    #print j
    #rotate the vectors
    Coffs2R1=Coffs2startR1*numpy.exp(1j*totalAngle)
    Coffs4R1=Coffs4startR1*numpy.exp(1j*totalAngle)
    Coffs2R2=Coffs2startR2*numpy.exp(1j*totalAngle)
    Coffs4R2=Coffs4startR2*numpy.exp(1j*totalAngle)
    Roach1.writeCoefficients(Coffs1startR1,Coffs2R1,Coffs3startR1,Coffs4R1)
    Roach2.writeCoefficients(Coffs1startR2,Coffs2R2,Coffs3startR2,Coffs4R2)
    time.sleep(sleepTime)
    ND.diodeOn()
    time.sleep(sleepTime)
    roach1Polon=Roach1.updatePolarisation()
    roach2Polon=Roach2.updatePolarisation()
    ND.diodeOff()
    time.sleep(sleepTime)
    roach1Poloff = Roach1.updatePolarisation()
    roach2Poloff = Roach2.updatePolarisation()
    dRef1RR[:,i]=(roach1Polon[0]-roach1Poloff[0])
    dRef1LL[:,i]=(roach1Polon[2]-roach1Poloff[2])
    dRef2RR[:,i]=(roach2Polon[0]-roach2Poloff[0])
    dRef2LL[:,i]=(roach2Polon[2]-roach2Poloff[2])
    i=i+1
    totalAngle=totalAngle+angleSize
    #print i
    print totalAngle



pylab.figure()
pylab.subplot(2,1,1)
pylab.plot(dRef1RR)
pylab.title('dRefRR Pumba')
pylab.subplot(2,1,2)
pylab.plot(dRef1LL)
pylab.title('dRefLL Pumba')

pylab.figure()
pylab.subplot(2,1,1)
pylab.plot(dRef2RR)
pylab.title('dRefRR Timon')
pylab.subplot(2,1,2)
pylab.plot(dRef2LL)
pylab.title('dRefLL Timon')
pylab.ylim([-2e11,8e11])


i=0
location1LL=numpy.zeros(64)
location1RR=numpy.zeros(64)
thetaNew1LL=numpy.zeros(64)
thetaNew1RR=numpy.zeros(64)

location2LL=numpy.zeros(64)
location2RR=numpy.zeros(64)
thetaNew2LL=numpy.zeros(64)
thetaNew2RR=numpy.zeros(64)
for i in range(64):
    a=numpy.abs((dRef1RR[i,:]))
    loc=a.argmax()
    location1RR[i]=loc
    thetaNew1RR[i]=angle[location1RR[i]]
    a=numpy.abs((dRef1LL[i,:]))
    loc=a.argmax()
    location1LL[i]=loc
    thetaNew1LL[i]=angle[location1LL[i]]
    a=numpy.abs((dRef2RR[i,:]))
    loc=a.argmax()
    location2RR[i]=loc
    thetaNew2RR[i]=angle[location2RR[i]]
    a=numpy.abs((dRef2LL[i,:]))
    loc=a.argmax()
    location2LL[i]=loc
    thetaNew2LL[i]=angle[location2LL[i]]
  
  


Coffs2R1=Coffs2startR1*numpy.exp(1j*thetaNew1RR)
Coffs4R1=Coffs4startR1*numpy.exp(1j*thetaNew1LL)
Coffs2R2=Coffs2startR2*numpy.exp(1j*thetaNew2RR)
Coffs4R2=Coffs4startR2*numpy.exp(1j*thetaNew2LL)


Roach1.writeCoefficients(Coffs1startR1,Coffs2R1,Coffs3startR1,Coffs4R1)
Roach2.writeCoefficients(Coffs1startR2,Coffs2R2,Coffs3startR2,Coffs4R2)


[Coffs1R1,Coffs2R1,Coffs3R1,Coffs4R1] = Roach1.readCoefficients()
Coffs1R1[0:7]=0
Coffs2R1[0:7]=0
Coffs3R1[0:7]=0
Coffs4R1[0:7]=0
Coffs1R1[61:64]=0
Coffs2R1[61:64]=0
Coffs3R1[61:64]=0
Coffs4R1[61:64]=0
Roach1.writeCoefficients(Coffs1R1,Coffs2R1,Coffs3R1,Coffs4R1)

[Coffs1R2,Coffs2R2,Coffs3R2,Coffs4R2] = Roach2.readCoefficients()
Coffs1R2[0:3]=0
Coffs2R2[0:3]=0
Coffs3R2[0:3]=0
Coffs4R2[0:3]=0
Coffs1R2[61:64]=0
Coffs2R2[61:64]=0
Coffs3R2[61:64]=0
Coffs4R2[61:64]=0
Roach2.writeCoefficients(Coffs1R2,Coffs2R2,Coffs3R2,Coffs4R2)


#[Coffs1timon,Coffs2timon,Coffs3timon,Coffstimon] = Timon.readCoefficients()
#Coffs2timon=Coffs2timon*numpy.exp(1j*thetaRtimon)
#Coffs4timon=Coffs4timon*numpy.exp(1j*thetaLtimon)
#Timon.writeCoefficients(Coffs1timon,Coffs2timon,Coffs3timon,Coffs4timon)


ND.diodeOn()

#rotate the channels by the following vector
#umba.rotateByVector(theta,theta,theta,theta)

time.sleep(sleepTime)
roach1Polon=Roach1.updatePolarisation()
roach2Polon=Roach2.updatePolarisation()
ND.diodeOff()
time.sleep(sleepTime)

roach1Poloff = Roach1.updatePolarisation()
roach2Poloff = Roach2.updatePolarisation()

ylimSmall=0.45*desiredResponse
ylimBig=0.6*desiredResponse
ypollimSmall = -1e12
ypollimBig = +1e12
pylab.figure()
pylab.subplots_adjust(hspace=1)
pylab.subplot(6,1,1)
pylab.plot(roach1Polon[0])
pylab.plot(roach1Poloff[0])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' RR Pumba')
pylab.subplot(6,1,2)
pylab.plot(roach1Polon[1])
pylab.plot(roach1Poloff[1])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Rload Pumba')
pylab.subplot(6,1,3)
pylab.plot(roach1Polon[2])
pylab.plot(roach1Poloff[2])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' LL Pumba')
pylab.subplot(6,1,4)
pylab.plot(roach1Polon[3])
pylab.plot(roach1Poloff[3])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Lload Pumba')
pylab.subplot(6,1,5)
pylab.plot(roach1Polon[4])
pylab.plot(roach1Poloff[4])
pylab.ylim([ypollimSmall,ypollimBig])
pylab.title(' Q Pumba')
pylab.subplot(6,1,6)
pylab.plot(roach1Polon[5])
pylab.plot(roach1Poloff[5])
pylab.ylim([ypollimSmall,ypollimBig])
pylab.title(' U Pumba')
pylab.savefig('pumba'+'/Polarisation2.png')

ylimSmall=0.45*desiredResponse
ylimBig=0.6*desiredResponse
ypollimSmall = -1e12
ypollimBig = +1e12
pylab.figure()
pylab.subplots_adjust(hspace=1)
pylab.subplot(6,1,1)
pylab.plot(roach2Polon[0])
pylab.plot(roach2Poloff[0])
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' RR Timon')
pylab.subplot(6,1,2)
pylab.plot(roach2Polon[1])
pylab.plot(roach2Poloff[1])
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Rload Timon')
pylab.subplot(6,1,3)
pylab.plot(roach2Polon[2])
pylab.plot(roach2Poloff[2])
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' LL Timon')
pylab.subplot(6,1,4)
pylab.plot(roach2Polon[3])
pylab.plot(roach2Poloff[3])
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Lload Timon')
pylab.subplot(6,1,5)
pylab.plot(roach2Polon[4])
pylab.plot(roach2Poloff[4])
#pylab.ylim([ypollimSmall,ypollimBig])
pylab.title(' Q Timon')
pylab.subplot(6,1,6)
pylab.plot(roach2Polon[5])
pylab.plot(roach2Poloff[5])
pylab.ylim([ypollimSmall,ypollimBig])
pylab.title(' U Timon')

pylab.savefig('timon'+'/Polarisation2.png')


##rotate the vectors using the roach coefficients to calculate Q and U rotation
j=0
i=0;
totalAngle=0
dRefQ1=numpy.zeros((64,256))
dRefU1=numpy.zeros((64,256))
dRefQ2=numpy.zeros((64,256))
dRefU2=numpy.zeros((64,256))
angle=numpy.zeros(256)
window_size = 3
window= numpy.ones(int(window_size))/float(window_size) # moving average window
[Coffs1startR1,Coffs2startR1,Coffs3startR1,Coffs4startR1] = Roach1.readCoefficients()
[Coffs1startR2,Coffs2startR2,Coffs3startR2,Coffs4startR2] = Roach2.readCoefficients()
totalAngle=0
angleSize=0.1
i=0
while totalAngle<6.3:
    j=angleSize
    angle[i]=totalAngle
    #print j
    #rotate the vectors
    Coffs1R1=Coffs1startR1*numpy.exp(1j*totalAngle)
    Coffs2R1=Coffs2startR1*numpy.exp(1j*totalAngle)
    Coffs1R2=Coffs1startR2*numpy.exp(1j*totalAngle)
    Coffs2R2=Coffs2startR2*numpy.exp(1j*totalAngle)
    Roach1.writeCoefficients(Coffs1startR1,Coffs2R1,Coffs3startR1,Coffs4R1)
    Roach2.writeCoefficients(Coffs1startR2,Coffs2R2,Coffs3startR2,Coffs4R2)
    time.sleep(sleepTime)
    ND.diodeOn()
    time.sleep(sleepTime)
    roach1Polon=Roach1.updatePolarisation()
    roach2Polon=Roach2.updatePolarisation()
    ND.diodeOff()
    time.sleep(sleepTime)
    roach1Poloff = Roach1.updatePolarisation()
    roach2Poloff = Roach2.updatePolarisation()
    dRefQ1[:,i]=(roach1Polon[4]-roach1Poloff[4])
    dRefU1[:,i]=(roach1Polon[5]-roach1Poloff[5])
    dRefQ2[:,i]=(roach2Polon[4]-roach2Poloff[4])
    dRefU2[:,i]=(roach2Polon[5]-roach2Poloff[5])
    i=i+1
    totalAngle=totalAngle+angleSize
    #print i
    print totalAngle


pylab.figure()
pylab.subplot(2,1,1)
pylab.plot(dRefQ1)
pylab.title('dRefQ Pumba')
pylab.subplot(2,1,2)
pylab.plot(dRefU1)
pylab.title('dRefU Pumba')

pylab.figure()
pylab.subplot(2,1,1)
pylab.plot(dRefQ2)
pylab.title('dRefQ Timon')
pylab.subplot(2,1,2)
pylab.plot(dRefU2)
pylab.title('dRefU Timon')


i=0
locationQ1=numpy.zeros(64)
thetaNew1RR=numpy.zeros(64)
locationQ2=numpy.zeros(64)
thetaNew2RR=numpy.zeros(64)
for i in range(64):
    a=((dRefQ1[i,:]))
   # a=numpy.convolve(a,window,'same')
    loc=a.argmax()
  
    locationQ1[i]=loc
     #   a=(dRefRR[i,0:numpy.floor(4/angleSize)])
     #   print location[i]
     #   polyVals = numpy.polyfit(a[location[i]-2:location[i]+2],angle[location[i]-2:location[i]+2],1)
       # thetaNewRR[i]=numpy.polyval(polyVals,0)
    thetaNew1RR[i]=angle[locationQ1[i]]
    a=((dRefQ2[i,:]))
   # a=numpy.convolve(a,window,'same')
    loc=a.argmax()
  
    locationQ2[i]=loc
     #   a=(dRefRR[i,0:numpy.floor(4/angleSize)])
     #   print location[i]
     #   polyVals = numpy.polyfit(a[location[i]-2:location[i]+2],angle[location[i]-2:location[i]+2],1)
       # thetaNewRR[i]=numpy.polyval(polyVals,0)
    thetaNew2RR[i]=angle[locationQ2[i]]
 
 

 
Coffs2R1=Coffs2startR1*numpy.exp(1j*thetaNew1RR)
Coffs1R1=Coffs1startR1*numpy.exp(1j*thetaNew1RR)
Coffs2R2=Coffs2startR2*numpy.exp(1j*thetaNew2RR)
Coffs1R2=Coffs1startR2*numpy.exp(1j*thetaNew2RR)


Roach1.writeCoefficients(Coffs1R1,Coffs2R1,Coffs3startR1,Coffs4startR1)
Roach2.writeCoefficients(Coffs1R2,Coffs2R2,Coffs3startR2,Coffs4startR2)

ND.diodeOn()

#rotate the channels by the following vector
#umba.rotateByVector(theta,theta,theta,theta)

time.sleep(sleepTime)
roach1Polon=Roach1.updatePolarisation()
roach2Polon=Roach2.updatePolarisation()
ND.diodeOff()
time.sleep(sleepTime)

roach1Poloff = Roach1.updatePolarisation()
roach2Poloff = Roach2.updatePolarisation()

ylimSmall=0.45*desiredResponse
ylimBig=0.9*desiredResponse
ypollimSmall = -1e12
ypollimBig = +1e12
pylab.figure()
pylab.subplots_adjust(hspace=1)
pylab.subplot(6,1,1)
pylab.plot(roach1Polon[0])
pylab.plot(roach1Poloff[0])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' RR Pumba')
pylab.subplot(6,1,2)
pylab.plot(roach1Polon[1])
pylab.plot(roach1Poloff[1])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Rload Pumba')
pylab.subplot(6,1,3)
pylab.plot(roach1Polon[2])
pylab.plot(roach1Poloff[2])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' LL Pumba')
pylab.subplot(6,1,4)
pylab.plot(roach1Polon[3])
pylab.plot(roach1Poloff[3])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Lload Pumba')
pylab.subplot(6,1,5)
pylab.plot(roach1Polon[4])
pylab.plot(roach1Poloff[4])
pylab.ylim([ypollimSmall,ypollimBig])
pylab.title(' Q Pumba')
pylab.subplot(6,1,6)
pylab.plot(roach1Polon[5])
pylab.plot(roach1Poloff[5])
pylab.ylim([ypollimSmall,ypollimBig])
pylab.title(' U Pumba')
pylab.savefig('pumba'+'/Polarisation3.png')

ylimSmall=0.45*desiredResponse
ylimBig=0.9*desiredResponse
ypollimSmall = -1e12
ypollimBig = +1e12
pylab.figure()
pylab.subplots_adjust(hspace=1)
pylab.subplot(6,1,1)
pylab.plot(roach2Polon[0])
pylab.plot(roach2Poloff[0])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' RR Timon')
pylab.subplot(6,1,2)
pylab.plot(roach2Polon[1])
pylab.plot(roach2Poloff[1])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Rload Timon')
pylab.subplot(6,1,3)
pylab.plot(roach2Polon[2])
pylab.plot(roach2Poloff[2])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' LL Timon')
pylab.subplot(6,1,4)
pylab.plot(roach2Polon[3])
pylab.plot(roach2Poloff[3])
pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Lload Timon')
pylab.subplot(6,1,5)
pylab.plot(roach2Polon[4])
pylab.plot(roach2Poloff[4])
pylab.ylim([ypollimSmall,ypollimBig])
pylab.title(' Q Timon')
pylab.subplot(6,1,6)
pylab.plot(roach2Polon[5])
pylab.plot(roach2Poloff[5])
pylab.ylim([ypollimSmall,ypollimBig])
pylab.title(' U Timon')

pylab.savefig('timon'+'/Polarisation3.png')


[Coffs1endR1,Coffs2endR1,Coffs3endR1,Coffs4endR1] = Roach1.readCoefficients()
[Coffs1endR2,Coffs2endR2,Coffs3endR2,Coffs4endR2] = Roach2.readCoefficients()
Roach1.writeCoefficients(Coffs1endR1,Coffs2endR1,Coffs3endR1,Coffs4endR1)
Roach2.writeCoefficients(Coffs1endR2,Coffs2endR2,Coffs3endR2,Coffs4endR2)

Roach1.saveCoefficientstoDisk('pumba')
Roach2.saveCoefficientstoDisk('timon')

thetaNew1RR=numpy.zeros(64)
thetaNew1RR[18]=3.14
thetaNew1RR[29]=3.14
Coffs2R2a=Coffs2endR2*numpy.exp(1j*thetaNew1RR)
Coffs1R2a=Coffs1endR2*numpy.exp(1j*thetaNew1RR)
Roach2.writeCoefficients(Coffs1R2a,Coffs2R2a,Coffs3endR2,Coffs4endR2)


thetaNew1RR=numpy.zeros(64)
thetaNew1RR[:]=3.14
Coffs2R1a=Coffs2endR1*numpy.exp(1j*thetaNew1RR)
Roach1.writeCoefficients(Coffs1endR1,Coffs2R1a,Coffs3endR1,Coffs4endR1)
Coffs2R2a=Coffs2endR2*numpy.exp(1j*thetaNew1RR)
Roach2.writeCoefficients(Coffs1endR2,Coffs2R2a,Coffs3endR2,Coffs4endR2)


Roach1.fpga.stop()
Roach2.fpga.stop()
