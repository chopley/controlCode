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
pylab.ion()

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


pylab.show()



