#!/usr/bin/env python
'''
This script demonstrates programming an FPGA, configuring a wideband spectrometer and plotting the received data using the Python KATCP library along with the katcp_wrapper distributed in the corr package. Designed for use with TUT3 at the 2009 CASPER workshop.\n

You need to have KATCP and CORR installed. Get them from http://pypi.python.org/pypi/katcp and http://casper.berkeley.edu/svn/trunk/projects/packetized_correlator/corr-0.4.0/

\nAuthor: Jason Manley, November 2009.
'''

#TODO: add support for ADC histogram plotting.
#TODO: add support for determining ADC input level 

import corr,time,numpy,struct,sys,logging,pylab,serial

katcp_port=7147
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
    fpga.write_int(snapctrl,0)
    fpga.write_int(snapctrl,5)
    fpga.write_int(snapctrl,7)	
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

    


    print 'Configuring accumulation period...',
    fpga.write_int('acc_len',opts.acc_len)
    print 'done',str(opts.acc_len)

    print 'Resetting counters...',
    fpga.write_int('cnt_rst',1) 
    fpga.write_int('ctrl_sw',43690) #fft size
    fpga.write_int('cnt_rst',0) 
#    fpga.write_int('snap_we',0)
#    fpga.write_int('snap_we',1)
#    fpga.write_int('sync_count',2**27-1)

    fpga.write_int('acc_len',78125);
    #fpga.write_int('real',781)
    #fpga.write_int('imag',0)
	


    print 'done'

except KeyboardInterrupt:
    exit_clean()
except:
    exit_fail()

exit_clean()

