add_plot 281 53 {actual az/el} 0 0.0142598528047809 1 1 maximum 20.0 array.frame.utc.date {} {
  graph 2.64088007464721 28.7750166070035 az {{antenna0.tracker.actual[0][0]} {}} {} 0 1 1 0
  graph 8.03992091601585 31.2917486498941 el {{antenna0.tracker.actual[1][0]} {}} {} 0 1 1 0
  graph -18.7323534179688 18.8808983398438 AzErr {{antenna0.servo.fast_az_err[0][0]} {} {}} {} 0 1 1 0
  graph -20.8623763671875 20.4311810546875 ElError {{antenna0.servo.fast_el_err[0][0]} {}} {} 0 1 1 0
  graph -101633.34296875 4022270.42109375 Roach1RR {{antenna0.roach2.RRfreq[0][0]} {} {} {}} {} 0 1 1 0
  graph 103274.05 106946.95 {} {{antenna0.roach2.RR[32][0]} {}} {} 0 1 1 0
} normal 0 0.0 1
