# -*- coding: utf-8 -*-
"""
Created on Sat Sep 21 04:55:33 2013

@author: cbassuser
"""


def updateRoachReadout(nSamples,sleepTime,Roach1,Roach2):
    ### function to do the sample readout and sample accumulation
    print 'Starting Sample...'    
    time.sleep(sleepTime)
    ## first get the data from both of the roaches
    roach1Pol=Roach1.updatePolarisation() 
    roach2Pol=Roach2.updatePolarisation()
    r1accum=roach1Pol
    r2accum=roach2Pol
    ###next accumulate for the appropriate number of integrations
    for k in range(0,nSamples):
       # print i
        time.sleep(sleepTime)
        r1=Roach1.updatePolarisation()
        r2=Roach2.updatePolarisation()
        r1accum=numpy.add(r1accum,r1)
        r2accum=numpy.add(r2accum,r2)
    r1accum=r1accum/nSamples
    r2accum=r2accum/nSamples
    roach1Pol = r1accum
    roach2Pol=r2accum
    print 'Completed Sample...'
    return [roach1Pol,roach2Pol]

import copy,pylab,time,numpy,cbass,sys,os,struct# first import 


Roach1=cbass.roach()
Roach2=cbass.roach()
ND=cbass.roach()
nIntegrations=781250 #number roach integrations in a sample- 78125 ~10ms integrations
sleepTime=(nIntegrations/78125)*0.03
desiredResponse = 1e13
nSamples=5; ##number of samples-i.e.  I accumulate nSamples of nIntegrations


Roach1.instantiateRoach('pumba','rx_10dec_stat_2013_Jan_11_1059.bof',7147,4,nIntegrations)
Roach2.instantiateRoach('timon','rx_10dec_stat_2013_Jan_11_1059.bof',7147,4,nIntegrations)
ND.instantiateRoach('pumba','rx_10dec_stat_2013_Jan_11_1059.bof',7147,4,nIntegrations)


[Coffs1endR1,Coffs2endR1,Coffs3endR1,Coffs4endR1] = Roach1.readCoefficientsFromDisk('pumba')
[Coffs1endR2,Coffs2endR2,Coffs3endR2,Coffs4endR2] = Roach2.readCoefficientsFromDisk('timon')

Roach1.writeCoefficients(Coffs1endR1,Coffs2endR1,Coffs3endR1,Coffs4endR1)
Roach2.writeCoefficients(Coffs1endR2,Coffs2endR2,Coffs3endR2,Coffs4endR2)

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
[roach1Polon,roach2Polon]=updateRoachReadout(nSamples,sleepTime,Roach1,Roach2)
ND.diodeOff()
[roach1Poloff,roach2Poloff]=updateRoachReadout(nSamples,sleepTime,Roach1,Roach2)



ylimSmall=10*numpy.log10(desiredResponse)-0.1
ylimBig=10*numpy.log10(desiredResponse)+0.2
ypollimSmall = -1e12
ypollimBig = +1e12



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
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' LL Pumba')
pylab.subplot(6,1,4)
pylab.plot(10*numpy.log10(roach1Polon[3]))
pylab.plot(10*numpy.log10(roach1Poloff[3]))
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Lload Pumba')
pylab.subplot(6,1,5)
pylab.plot(roach1Polon[4])
pylab.plot(roach1Poloff[4])
#pylab.ylim([ypollimSmall,ypollimBig])
pylab.title(' Q Pumba')
pylab.subplot(6,1,6)
pylab.plot(roach1Polon[5])
pylab.plot(roach1Poloff[5])
#pylab.ylim([ypollimSmall,ypollimBig])
pylab.title(' U Pumba')
pylab.show()


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
#pylab.ylim([ypollimSmall,ypollimBig])
pylab.title(' U Timon')

pylab.figure()

pylab.subplot(6,1,1)
pylab.plot(roach2Polon[0]-roach2Poloff[0])
pylab.subplot(6,1,2)
pylab.plot(roach2Polon[1]-roach2Poloff[1])


pylab.show()



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
Coffs4R1=Coffs4startR1
Coffs4R2=Coffs4startR2
Coffs2R1=Coffs2startR1
Coffs2R2=Coffs2startR2
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
    [roach1Polon,roach2Polon]=updateRoachReadout(nSamples,sleepTime,Roach1,Roach2)
    ND.diodeOff()
    time.sleep(sleepTime)
    [roach1Poloff,roach2Poloff]=updateRoachReadout(nSamples,sleepTime,Roach1,Roach2)
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
