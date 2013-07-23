# ANSI-C++ compiler.

CC = g++

# C compilation flags.

CFLAGS = -g -Wall -Wmissing-prototypes -Wmissing-declarations \
	-Dlinux_i486_gcc -D_BSD_SOURCE -D_POSIX_C_SOURCE=199506L \
	-D__EXTENSIONS__  

#-----------------------------------------------------------------------
# The configuration file must define the following:
#
# CC
# CCLIB
#
#-----------------------------------------------------------------------
# List the default targets.
#-----------------------------------------------------------------------

BINS = incrhex printhex username 
default: $(BINS)

#-----------------------------------------------------------------------
# Make targets.
#-----------------------------------------------------------------------

incrhex: incrhex.o
	$(CC) -o incrhex incrhex.o 

printhex: printhex.o
	$(CC) -o printhex printhex.o 

username: username.o
	$(CC) -o username username.o 

#-----------------------------------------------------------------------
# Clean up
#-----------------------------------------------------------------------

clean_o:
	rm *.o

clean_bins:
	rm $(BINS)

clean: clean_o clean_bins
