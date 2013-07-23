slew az=82.5, el=15.16
until $acquired(source)

mark add, f0
slew az=82.5, el=83
until $acquired(source)
mark remove, f0

slew az=262.5, el=83
until $acquired(source)


mark add, f0
slew az=262.5, el=15.16
until $acquired(source)
mark remove, f0