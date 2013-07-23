add_plot 122 273 Metrology 0 0.00188434829537 1 0 maximum 20.0 {antenna0.tracker.utc.date[0]} {} {
  graph -0.475 9.975 {LGDT (mm)} {{antenna0.tracker.linear_sensor[0-3]} {antenna0.tracker.linear_sensor_avg[0-3]} {}} {} 0
  graph -1.53091015625 27.71753125 {tilt (arcsec)} {{antenna0.tracker.tilt_xy[0-1]} {antenna0.tracker.tilt_xy_avg[0-1]} {}} {} 0
  graph 126.756724218002 143.582182323491 AZ {{antenna0.tracker.actual[0]} {} {}} {} 0
  graph 54.9970196648344 54.9970196650786 EL {{antenna0.tracker.actual[1]} {} {}} {} 0
} normal 0 0.0
