add_plot 90 52 {Pointing telescope temperature} 0 0.428171296298 1 1 maximum 20.0 array.frame.utc.date {} {
  graph 9.206375 26.666125 temp {{array.pointingTel.AD_Scaled_Values[0-2]} {} {} {} {} {} {array.pointingTel.PIDSetpoints[0-2]} {} {} {} {}} {} 0 1 1
  graph -0.05 1.05 Heat {{array.pointingTel.PIDDutyCycles[0-2]} {} {} {} {}} {} 0 1 1
} normal 0 0.0 1
