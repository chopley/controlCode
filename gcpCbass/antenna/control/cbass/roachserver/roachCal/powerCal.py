# -*- coding: utf-8 -*-
"""
Created on Sat Sep 21 04:51:46 2013

@author: cbassuser
"""

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
pylab.ion()

#Roach.instantiateRoach(roachName,'cbassrx_2mar_pow_2012_Mar_02_1638.bof',7147,10,nIntegrations)
#ND.instantiateRoach('pumba','cbassrx_2mar_pow_2012_Mar_02_1638.bof',7147,10,nIntegrations) ##we always use pumba for the noise diode!!

Roach1.instantiateRoach('pumba','cbassrx_2may_pow_2012_May_02_1555.bof',7147,10,nIntegrations)
Roach2.instantiateRoach('timon','cbassrx_2may_pow_2012_May_02_1555.bof',7147,10,nIntegrations)
ND.instantiateRoach('pumba','cbassrx_2may_pow_2012_May_02_1555.bof',7147,10,nIntegrations) ##we always use pumba for the noise diode!!


#initialise the class coefficients and those stored on the roach
ND.initialiseCoefficients(2000)
Roach1.initialiseCoefficients(2000)
Roach2.initialiseCoefficients(100)


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
ylimSmall=10*numpy.log10(desiredResponse)-0.1
ylimBig=10*numpy.log10(desiredResponse)+0.2

pylab.figure()
pylab.subplots_adjust(hspace=0.5)
pylab.subplot(411)
pylab.plot(freq,(fpga1Poweron[0]))
pylab.plot(freq,(fpga1Poweroff[0]))
pylab.grid()
pylab.ylabel('dB')
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel1 Pumba')
pylab.subplot(412)
pylab.plot(freq,(fpga1Poweron[1]))
pylab.plot(freq,(fpga1Poweroff[1]))
pylab.grid()
pylab.ylabel('dB')
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel2 Pumba')
pylab.subplot(413)
pylab.plot(freq,(fpga1Poweron[2]))
pylab.plot(freq,(fpga1Poweroff[2]))
pylab.grid()
pylab.ylabel('dB')
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel3 Pumba')
pylab.subplot(414)
pylab.plot(freq,(fpga1Poweron[3]))
pylab.plot(freq,(fpga1Poweroff[3]))
pylab.grid()
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel4 Pumba')
pylab.savefig('pumba'+'/PowerPumba.png')
pylab.xlabel('Frequency [GHz]')
pylab.ylabel('dB')
pylab.draw()


freq=numpy.arange(5.5,5,-0.5/64)
ylimSmall=10*numpy.log10(desiredResponse)-0.1
ylimBig=10*numpy.log10(desiredResponse)+0.2
pylab.figure()
pylab.subplots_adjust(hspace=0.5)
pylab.subplot(411)
pylab.plot(freq,(fpga2Poweron[0]))
pylab.plot(freq,(fpga2Poweroff[0]))
pylab.grid()
pylab.ylabel('dB')
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel1 Timon')
pylab.subplot(412)
pylab.plot(freq,(fpga2Poweron[1]))
pylab.plot(freq,(fpga2Poweroff[1]))
pylab.grid()
pylab.ylabel('dB')
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel2 Timon')
pylab.subplot(413)
pylab.plot(freq,(fpga2Poweron[2]))
pylab.plot(freq,(fpga2Poweroff[2]))
pylab.grid()
pylab.ylabel('dB')
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel3 Timon')
pylab.subplot(414)
pylab.plot(freq,(fpga2Poweron[3]))
pylab.plot(freq,(fpga2Poweroff[3]))
pylab.grid()
#pylab.ylim([ylimSmall,ylimBig])
pylab.title(' Channel4 Timon')
pylab.savefig('timon'+'/PowerTimon.png')
pylab.xlabel('Frequency [GHz]')
pylab.ylabel('dB')
pylab.draw()

#and store the coefficients for later
[Coffs1PowerR1,Coffs2PowerR1,Coffs3PowerR1,Coffs4PowerR1] = Roach1.readCoefficients()
[Coffs1PowerR2,Coffs2PowerR2,Coffs3PowerR2,Coffs4PowerR2] = Roach2.readCoefficients()

Roach1.saveCoefficientstoDisk('pumba')
Roach2.saveCoefficientstoDisk('timon')
pylab.show()
