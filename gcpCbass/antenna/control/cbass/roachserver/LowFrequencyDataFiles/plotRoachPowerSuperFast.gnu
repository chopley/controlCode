set terminal postscript color
set output "RoachPower.ps"
#set terminal png size 1100,800

#set output "RoachPower.png"
timestart=330000
deltaT=1
#set xrange [timestart:timestart+deltaT]

#set xrange [10:10.2]
#set yrange [34:35]
set grid
set datafile separator ","
file1 = "SigGenClock_NoiseDiode.txt"
file2 = "SigGenClock_NoNoiseDiode.txt"
file3 = "SigGenClock_NoNoiseDiode_ValonOn.txt"
file4 = "ValonClock_NoiseDiode_ValonPoweredOffSwitchmode.txt"
file5 = "ValonClock_NoNoiseDiode_ValonPoweredOffSwitchmode.txt"
file6 = "ValonClock_NoNoiseDiode_ValonPoweredOffSwitchmode_b.txt"
file7 = "ValonClock_NoNoiseDiode_ValonPoweredOffSwitchmode_c.txt"
#set yrange [-15.8:-15.4]
#set yrange [-22:-20]
#set yrange [129.5:130.2]
#set yrange [131.2:132]
#set yrange [38:40]
set xlabel 'Seconds'
set ylabel 'Power [dBm]'
offset =0.1
dBoffset =0
set title 'Comparison'
#plot file3 using (0.01*$2*$5/78125):(10*log10($1)) ps 0.2 t 'Before',file5 using (0.01*$2*$5/78125):(10*log10($1)) ps 0.2 t 'After'
plot file3 using (0.01*$2*$5/78125):(10*log10($1)) ps 0.2 t 'Before',file7 using (0.01*$2*$5/78125):(10*log10($1)) ps 0.2 t 'After'
#plot filename using ($1+dBoffset) ps 0.2 t 'Channel1',filename using ($2+dBoffset) ps 0.2 t 'Channel1'
