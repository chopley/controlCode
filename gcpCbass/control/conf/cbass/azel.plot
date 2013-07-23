add_plot 376 121 {actual az/el} 0 0.0034722 1 1 maximum 20.0 array.frame.utc.date {} {
  graph 0 1 az {{antenna0.tracker.actual[0][0]} {}} {}
  graph 0 1 el {{antenna0.tracker.actual[1][0]} {}} {}
}
