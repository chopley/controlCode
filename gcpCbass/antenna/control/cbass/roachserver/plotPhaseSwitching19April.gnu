set terminal postscript enhanced color
set title 'Phase Switch Tests'
set xlabel 'Time [ns]'
set ylabel 'ADC Sample Value'
set output 'PhaseSwitch19April.ps'
set xrange [80:220]
file = "ADCSamplesPos.txt"
file2 = "ADCSamplesNeg.txt"
set grid 
plot file with lines
plot file2 with lines
