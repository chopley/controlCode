set terminal postscript color
set output "RoachPowerPS.ps"
#set terminal png size 1100,800

#set output "RoachPower.png"
timestart=330000
deltaT=1
#set xrange [timestart:timestart+deltaT]

#set xrange [350:*]
#set yrange [60:65]
set grid
set datafile separator ","
filename = "dataNotPhaseSwitchedHighSpeed.txt"
#set yrange [-15.8:-15.4]
#set yrange [-22:-20]
#set yrange [129.5:130.2]
#set yrange [131.2:132]
#set yrange [38:40]
set xlabel 'Seconds'
set ylabel 'Power [ADC]'
offset =0.1
dBoffset =0
set title 'Not Phase Switched'
plot filename using (0.01*$2*$5/78125):($1) ps 0.2 t 'Channel1'
#plot filename using ($1+dBoffset) ps 0.2 t 'Channel1',filename using ($2+dBoffset) ps 0.2 t 'Channel1'

filename = "dataPhaseSwitchedHighSpeed.txt"
set xlabel 'Seconds'
set ylabel 'Power [ADC]'
offset =0.1
dBoffset =0
set title 'PhaseSwitched'
plot filename using (0.01*$2*$5/78125):($1) ps 0.2 t 'Channel1'
