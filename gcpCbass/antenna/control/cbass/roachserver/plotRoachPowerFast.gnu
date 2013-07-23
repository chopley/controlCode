set terminal postscript color
set output "RoachPower.ps"
#set terminal png size 1100,800

#set output "RoachPower.png"
timestart=330000
deltaT=1
#set xrange [timestart:timestart+deltaT]
#set xrange [*:*]
set grid
set datafile separator ","
filename = "roachPower.txt"
#set yrange [-15.8:-15.4]
#set yrange [-22:-20]
#set yrange [129.5:130.2]
set yrange [97.5:97.6]
set xlabel 'Seconds'
set ylabel 'Power [dBm]'
offset =0.1
dBoffset =0
set title 'Receiver Noise Temperature'
plot filename using ($1+dBoffset) ps 0.2 t 'Channel1'
plot filename using ($2+dBoffset) ps 0.2 t 'Channel2'
plot filename using ($3+dBoffset) ps 0.2 t 'Channel3'
plot filename using ($4+dBoffset) ps 0.2 t 'Channel4'
#plot filename using ($1+dBoffset) ps 0.2 t 'Channel1',filename using ($2+dBoffset) ps 0.2 t 'Channel1'
