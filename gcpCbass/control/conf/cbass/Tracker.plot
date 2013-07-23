add_plot 301 182 {Tracking error} 0 0.00461464268693 1 1 maximum 20.0 {antenna0.tracker.utc.date[0]} {} {
  graph -.005 .005 {AZ error} {{antenna0.tracker.errors[0]} {} {} {} {} {} {} {} {} {}} {} 0
  graph -.005 .005 {EL error} {{antenna0.tracker.errors[1]} {} {} {} {} {}} {} 0
  graph 0 360 AZ {{antenna0.tracker.actual[0]} {antenna0.tracker.expected[0]} {} {} {} {} {} {} {} {}} {} 0
  graph 0 95 EL {{antenna0.tracker.actual[1]} {antenna0.tracker.expected[1]} {} {} {}} {} 0
  graph -1.17549435082229e-34 1.17549435082229e-34 {AZ scan} {{antenna0.tracker.scan_off[0]} {}} {} 0
  graph -1.17549435082229e-34 1.17549435082229e-34 {EL offset} {{antenna0.tracker.horiz_off[1]} {}} {} 0
  graph -1.17549435082229e-34 1.17549435082229e-34 {inv seq} {{antenna0.tracker.invalidSeq[0]} {} {} {}} {} 0
  graph 0.25 0.3 UC {{array.fridge.Temperature[0]} {} {}} {} 0
} normal 0 0.0
