add_plot 481 489 Weather 0 0.0013503086498 1 1 maximum 20.0 array.frame.utc.date {} {
  graph -70 -40 {air temp} {{array.weather.airTemperature[0]} {} {} {}} {} 0
  graph 0 100 RH {{array.weather.relativeHumidity[0]} {}} {} 0
  graph 660 680 pressure {{array.weather.pressure[0]} {} {} {} {}} {} 0
  graph 0 10 {wind speed} {{array.weather.windSpeed[0]} {} {} {}} {} 0
  graph 0 360 {wind dir} {{array.weather.windDirection[0]} {}} {} 0
} normal 0 0.0
