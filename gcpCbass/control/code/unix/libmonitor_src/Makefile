# This file is designed to be invoked from a recursive call to make
# from the makefile in SZA/code/unix/.
#
# The makefile that includes it is expected to define the following
# macros:
#
#  INC       - The full name of the SZA/code/unix/include directory.
#  LIB       - The full name of the SZA/code/unix/lib directory.
#  SHARE     - The full name of the SZA/code/share/include directory.
#  CC        - The C compiler to be used.
#  CFLAGS    - Flags for the C compiler.
#  RANLIB    - The archiver randomizer command (use : if not needed).
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# Generic rule for making .o files
#-----------------------------------------------------------------------

%.o: %.c
	$(CC) $(CFLAGS) $(INCFLAGS) -c -o $@ $<

#-----------------------------------------------------------------------
# List the default targets.
#-----------------------------------------------------------------------

LIBSO = $(GCPLIB)/libGcpMonitor$(LIBSO_SUFFIX)
all: $(LIBSO) $(INC)/monitor_stream.h $(INC)/im_monitor_stream.h

clean: clean_lib clean_h clean_depend

#-----------------------------------------------------------------------
# Library generation.
#-----------------------------------------------------------------------

MONITOR_OBJ = monitor_stream.o im_monitor_stream.o net_monitor.o \
 net_im_monitor.o file_monitor.o

$(LIBSO): depend $(MONITOR_OBJ)
	$(CC) $(LIBSO_FLAGS) -o $(LIBSO) $(MONITOR_OBJ)
	$(RANLIB) $@

clean_lib:
	/bin/rm -f $(MONITOR_OBJ) $(LIB)/libmonitor.a $(LIBSO)

#-----------------------------------------------------------------------
# Arrange for copies of pertinent include files to be visible in the
# shared include directory.
#-----------------------------------------------------------------------

$(INC)/monitor_stream.h: monitor_stream.h
	cp monitor_stream.h $@

$(INC)/im_monitor_stream.h: im_monitor_stream.h
	cp im_monitor_stream.h $@

clean_h:
	/bin/rm -f $(INC)/monitor_stream.h $(INC)/im_monitor_stream.h

#-----------------------------------------------------------------------
# Include the file in which automatic dependencies are deposited by
# make depend, above.  But only if it exists.  If not, the rule for
# depend, below, will make it.  Don't include Makefile.rules until
# after the desired default target is defined, or Makefile.rules will
# determine the default target for you!
#-----------------------------------------------------------------------

MAKERULES := $(wildcard Makefile.rules)

ifeq ($(MAKERULES), Makefile.rules)
	RULES_EXISTS = y
	include Makefile.rules
else
	RULES_EXISTS = n
endif

#-----------------------------------------------------------------------
# Make dependencies, if they don't already exist
#-----------------------------------------------------------------------

RULESFILES =  $(wildcard *.c)

# If Makefile.rules doesn't exist (see above), executing this rule
# will cause it to be made and sourced.

depend:
	@echo 'Executing depend rule'
ifeq ($(RULES_EXISTS),n)
	$(CC) $(CFLAGS) $(OPTIONS) $(INCFLAGS) -MM $(RULESFILES) > Makefile.rules
	@set -e; [ -s Makefile.rules ] ;
endif

clean_depend:
	/bin/rm -f Makefile.rules
