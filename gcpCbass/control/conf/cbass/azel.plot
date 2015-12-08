add_plot 0 51 {actual az/el} 0 0.174201388902 1 1 maximum 20.0 array.frame.utc.date {} {
  graph -292.730053857973 147.054571471807 az {{antenna0.tracker.actual[0][0]} {}} {} 0 1 1 0
  graph 10.549236731775 89.6586257133329 el {{antenna0.tracker.actual[1][0]} {}} {} 0 1 1 0
  graph -498.95 10499.95 Version {{antenna0.roach1.version[0][0]} {antenna0.roach2.version[0][0]} {} {}} {} 0 1 1 0
  graph -120.71896796875 111.89492109375 PosError {{antenna0.servo.fast_el_err[0][0]} {antenna0.servo.fast_az_err[0][0]} {} {}} {} 0 1 1 0
  graph -301451.85 6846124.85 RR {{antenna0.roach1.RR[32][0]} {antenna0.roach1.RR[16][0]} {antenna0.roach2.RR[32][0]} {antenna0.roach2.RR[16][0]} {antenna0.roach1.RRfreq[0][0]} {antenna0.roach2.RRfreq[0][0]} {} {} {} {}} {} 0 1 1 0
  graph -211174.75 4862393.75 LoadR {{antenna0.roach2.load1[32][0]} {antenna0.roach2.load1[16][0]} {antenna0.roach1.load1[32][0]} {antenna0.roach1.load1[16][0]} {antenna0.roach1.load1freq[0][0]} {antenna0.roach2.load1freq[0][0]} {} {} {} {} {} {}} {} 0 1 1 0
  graph -197299.1 4936293.1 LL {{antenna0.roach1.LL[16][0]} {antenna0.roach2.LL[16][0]} {antenna0.roach1.LLfreq[0][0]} {antenna0.roach2.LLfreq[0][0]} {} {} {} {} {} {} {} {} {} {}} {} 0 1 1 0
  graph -106517.2 2829497.2 LoadL {{antenna0.roach1.load2[16][0]} {antenna0.roach2.load2[16][0]} {antenna0.roach1.load2freq[0][0]} {antenna0.roach2.load2freq[0][0]} {} {} {} {} {} {} {}} {} 0 1 1 0
  graph -6285.1 5641.1 {} {{antenna0.servo.az_pid1[0][0]} {antenna0.servo.az_pid2[0][0]} {antenna0.servo.el_pid1[0][0]} {antenna0.servo.el_pid2[0][0]} {}} {} 0 1 1 0
  graph -197.97 197.37 {} {{antenna0.tracker.scan_off[0-1][0]} {}} {} 0 1 1 0
} normal 0 0.0 1
