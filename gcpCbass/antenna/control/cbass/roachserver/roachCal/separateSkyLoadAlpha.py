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
sleepTime=(nIntegrations/78125)*0.01 # was 0.03
desiredResponse = 5e12
nSamples=4; ##number of samples-i.e.  I accumulate nSamples of nIntegrations
pylab.ion()

Roach1.instantiateRoach('pumba','rx_10dec_stat_2013_Jan_11_1059.bof',7147,4,nIntegrations)
Roach2.instantiateRoach('timon','rx_10dec_stat_2013_Jan_11_1059.bof',7147,4,nIntegrations)
ND.instantiateRoach('pumba','rx_10dec_stat_2013_Jan_11_1059.bof',7147,4,nIntegrations)


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

ND.initialiseCoefficients(100)
Roach1.initialiseCoefficients(100)
Roach2.initialiseCoefficients(100)



#[Coffs1endR1,Coffs2endR1,Coffs3endR1,Coffs4endR1] = Roach1.readCoefficientsFromDisk('pumba')
#[Coffs1endR2,Coffs2endR2,Coffs3endR2,Coffs4endR2] = Roach2.readCoefficientsFromDisk('timon')

#Roach1.writeCoefficients(Coffs1endR1,Coffs2endR1,Coffs3endR1,Coffs4endR1)
#Roach2.writeCoefficients(Coffs1endR2,Coffs2endR2,Coffs3endR2,Coffs4endR2)
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
pylab.plot((roach1Polon[0]))
pylab.plot((roach1Poloff[0]))
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' RR Pumba')
pylab.subplot(6,1,2)
pylab.plot((roach1Polon[1]))
pylab.plot((roach1Poloff[1]))
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Rload Pumba')
pylab.subplot(6,1,3)
pylab.plot((roach1Polon[2]))
pylab.plot((roach1Poloff[2]))
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' LL Pumba')
pylab.subplot(6,1,4)
pylab.plot((roach1Polon[3]))
pylab.plot((roach1Poloff[3]))
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
pylab.draw()


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


pylab.draw()


##rotate the vectors using the roach coefficients
j=0
i=0;
totalAngle=0
alpha1RR=numpy.zeros((64,256))
alpha1LL=numpy.zeros((64,256))
alpha2RR=numpy.zeros((64,256))
alpha2LL=numpy.zeros((64,256))
angle=numpy.zeros(256)
window_size = 3
window= numpy.ones(int(window_size))/float(window_size) # moving average window
[Coffs1startR1,Coffs2startR1,Coffs3startR1,Coffs4startR1] = Roach1.readCoefficients()
[Coffs1startR2,Coffs2startR2,Coffs3startR2,Coffs4startR2] = Roach2.readCoefficients()
totalAngle=0
angleSize=0.2
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
    [roach1Polon,roach2Polon]=updateRoachReadout(nSamples,sleepTime,Roach1,Roach2)
    ND.diodeOff()
    time.sleep(sleepTime)
    [roach1Poloff,roach2Poloff]=updateRoachReadout(nSamples,sleepTime,Roach1,Roach2)
    #calculate alphas for each of RR,LL on both roaches
    # Alpha defined by alpha = (1-delta2/delta1)/(1+delta2/delta1)
    # where delta2 = on-off for load channel and delta1 = on-off for sky channel
    # RR == [0]; Rload == [1]; LL = [2]; Lload = [3]
    delta1 = roach1Polon[0] - roach1Poloff[0]
    delta2 = roach1Polon[1] - roach1Poloff[1] 
    alpha1RR[:,i]=(1 - (delta2/delta1))/(1 + (delta2/delta1))

    delta1 = roach1Polon[2] - roach1Poloff[2]
    delta2 = roach1Polon[3] - roach1Poloff[3] 
    alpha1LL[:,i]=(1 - (delta2/delta1))/(1 + (delta2/delta1))

    delta1 = roach2Polon[0] - roach2Poloff[0]
    delta2 = roach2Polon[1] - roach2Poloff[1] 
    alpha2RR[:,i]=(1 - (delta2/delta1))/(1 + (delta2/delta1))

    delta1 = roach2Polon[2] - roach2Poloff[2]
    delta2 = roach2Polon[3] - roach2Poloff[3] 
    alpha2LL[:,i]=(1 - (delta2/delta1))/(1 + (delta2/delta1))

    i=i+1
    totalAngle=totalAngle+angleSize
    #print i
    print totalAngle    
    
    
    
thing = copy.deepcopy(i)

pylab.figure()
pylab.title('AlphaRR Pumba')
for i in range(64): 
    pylab.subplot(16,4,i)
    pylab.plot(alpha1RR[i,0:thing])
    pylab.ylim([-1,1])

pylab.figure()
pylab.title('AlphaLL Pumba')
for i in range(64): 
    pylab.subplot(16,4,i)
    pylab.plot(alpha1LL[i,0:thing])
    pylab.ylim([-1,1])

pylab.figure()
pylab.title('AlphaRR Timon')
for i in range(64): 
    pylab.subplot(16,4,i)
    pylab.plot(alpha2RR[i,0:thing])
    pylab.ylim([-1,1])

pylab.figure()
pylab.title('AlphaLL Timon')
for i in range(64): 
    pylab.subplot(16,4,i)
    pylab.plot(alpha2LL[i,0:thing])
    pylab.ylim([-1,1])

pylab.figure()
pylab.title('Alpha at zero angle offset')
pylab.subplot(4,1,1)
pylab.plot(alpha1RR[:,0])

pylab.subplot(4,1,2)
pylab.plot(alpha1LL[:,0])

pylab.subplot(4,1,3)
pylab.plot(alpha2RR[:,0])

pylab.subplot(4,1,4)
pylab.plot(alpha2LL[:,0])


#pylab.ylim([-2e11,8e11])

pylab.draw()

i=0
location1LL=numpy.zeros(64)
location1RR=numpy.zeros(64)
thetaNew1LL=numpy.zeros(64)
thetaNew1RR=numpy.zeros(64)
bestalpha1RR = numpy.zeros(64)
bestalpha1LL = numpy.zeros(64)

location2LL=numpy.zeros(64)
location2RR=numpy.zeros(64)
thetaNew2LL=numpy.zeros(64)
thetaNew2RR=numpy.zeros(64)
bestalpha2RR = numpy.zeros(64)
bestalpha2LL = numpy.zeros(64)

for i in range(64):
    a=((alpha1RR[i,:]))
    loc=a.argmax()
    location1RR[i]=loc
    thetaNew1RR[i]=angle[location1RR[i]]
    bestalpha1RR[i] = alpha1RR[i,loc];

    a=((alpha1LL[i,:]))
    loc=a.argmax()
    location1LL[i]=loc
    thetaNew1LL[i]=angle[location1LL[i]]
    bestalpha1LL[i] = alpha1LL[i,loc];

    a=((alpha2RR[i,:]))
    loc=a.argmax()
    location2RR[i]=loc
    thetaNew2RR[i]=angle[location2RR[i]]
    bestalpha2RR[i] = alpha2RR[i,loc];

    a=((alpha2LL[i,:]))
    loc=a.argmax()
    location2LL[i]=loc
    thetaNew2LL[i]=angle[location2LL[i]]
    bestalpha2LL[i] = alpha2LL[i,loc];
  
  


Coffs2R1=Coffs2startR1*numpy.exp(1j*thetaNew1RR)
Coffs4R1=Coffs4startR1*numpy.exp(1j*thetaNew1LL)
Coffs2R2=Coffs2startR2*numpy.exp(1j*thetaNew2RR)
Coffs4R2=Coffs4startR2*numpy.exp(1j*thetaNew2LL)

#Plot out the best alphas so far

pylab.figure()
pylab.subplot(4,1,1)
pylab.plot(bestalpha1RR)
pylab.plot(thetaNew1RR)

pylab.subplot(4,1,2)
pylab.plot(bestalpha1LL)
pylab.plot(thetaNew1LL)

pylab.subplot(4,1,3)
pylab.plot(bestalpha2RR)
pylab.plot(thetaNew2RR)

pylab.subplot(4,1,4)
pylab.plot(bestalpha2LL)
pylab.plot(thetaNew2LL)





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


[Coffs1endR1,Coffs2endR1,Coffs3endR1,Coffs4endR1] = Roach1.readCoefficients()
[Coffs1endR2,Coffs2endR2,Coffs3endR2,Coffs4endR2] = Roach2.readCoefficients()
Roach1.writeCoefficients(Coffs1endR1,Coffs2endR1,Coffs3endR1,Coffs4endR1)
Roach2.writeCoefficients(Coffs1endR2,Coffs2endR2,Coffs3endR2,Coffs4endR2)

Roach1.saveCoefficientstoDisk('pumba')
Roach2.saveCoefficientstoDisk('timon')
pylab.show()



