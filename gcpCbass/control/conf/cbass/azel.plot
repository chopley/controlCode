add_plot 422 100 {actual az/el} 0 0.00461479106982155 1 1 maximum 20.0 array.frame.utc.date {} {
  graph -106.894798353572 -74.2286337192245 az {{antenna0.tracker.actual[0][0]} {}} {} 0 1 1 0
  graph 28.0620804990787 50.7334417549563 el {{antenna0.tracker.actual[1][0]} {}} {} 0 1 1 0
  graph -30.9103497070313 30.7803165039062 AzErr {{antenna0.servo.fast_az_err[0][0]} {} {}} {} 0 1 1 0
  graph -15.6345188232422 5.52619997558594 ElError {{antenna0.servo.fast_el_err[0][0]} {}} {} 0 1 1 0
  graph 5698.1390625 9578.0796875 Roach1RR {{antenna0.roach1.RRfreq[0][0]} {} {}} {} 0 1 1 0
} normal 0 0.0 1
