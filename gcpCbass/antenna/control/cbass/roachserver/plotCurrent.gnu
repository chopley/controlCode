set terminal postscript eps enhanced color
set title 'Phase Switch Tests'
set xlabel 'Time [ns]'
set ylabel 'ADC Sample Value'
set output 'PhaseSwitch.eps'
set xrange [100:170]
file = "ADCSamples.txt"
set grid 
plot file with lines
