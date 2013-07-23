set terminal postscript enhanced color
set title 'Phase Switch Tests 2May'
set xlabel 'Time [ns]'
set ylabel 'ADC Sample Value'
set output 'PhaseSwitch2May.ps'
set xrange [100:200]
file = "ADCSamplesPos1.txt"
file2 = "ADCSamplesPos2.txt"
file3 = "ADCSamplesPos3.txt"
file4 = "ADCSamplesPos4.txt"
file5 = "ADCSamplesNeg1.txt"
file6 = "ADCSamplesNeg2.txt"
file7 = "ADCSamplesNeg3.txt"
file8 = "ADCSamplesNeg4.txt"
set grid 
plot file with lines t 'Pos Chan1'
plot file2 with lines t 'Pos Chan2'
plot file3 with lines t 'Pos Chan3'
plot file4 with lines t 'Pos Chan4'
plot file5 with lines t 'Neg Chan1'
plot file6 with lines t 'Neg Chan2'
plot file7 with lines t 'Neg Chan3'
plot file8 with lines t 'Neg Chan4'
