# This file is designed to be invoked from a recursive call to make
# from the makefile in SZA/code/unix/.
#
# The makefile that includes it is expected to define the following
# macros:
#
#  LIB       - The full name of the SZA/code/unix/lib directory.
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# List the default targets.
#-----------------------------------------------------------------------

all: $(LIB)/cmd.tcl

#-----------------------------------------------------------------------
# Make the scripts visible in the shared library directory.
#-----------------------------------------------------------------------

$(LIB)/cmd.tcl: cmd.tcl
	cp cmd.tcl $@

clean:
	/bin/rm -f $(LIB)/cmd.tcl
